import pandas as pd
import numpy as np
import pickle
import os
import matplotlib.pyplot as plt
import itertools
from multiprocessing import Pool
import pyarrow as pa
import pyarrow.parquet as pq


from utils import generate_min_max
from utils import groupby_machine_informations
from utils import make_decision


# Function to optimize the memoryid data type
def optimize_memoryid_dtype(df):
    df['memoryid'] = df['memoryid'].astype('int64')
    return df

# Function to load data and merge category
def load_and_merge_data():
    # Load category data
    try:
        category = pickle.load(open('manual_category.pkl', 'rb'))
    except:
        category = pickle.load(open('category.pkl', 'rb'))
    category = optimize_memoryid_dtype(category)
    permanancy = pickle.load(open('category.pkl', 'rb'))

    # Merge category with permanancy
    category = pd.merge(category, permanancy, on=['sid', 'memoryid'], suffixes=('', '_y'), how='right')
    category['category'] = category['category'].fillna(category['category_y'])
    category = category.drop([col for col in category.columns if col.endswith('_y')], axis=1)

    # Load data
    with open('data_df.pkl', 'rb') as f:
        df_original = pickle.load(f)
    with open('trouble_tickets.pkl', 'rb') as f:
        trouble_tickets = pickle.load(f)

    return df_original, category, trouble_tickets

def preprocess_data(df_original, category, trouble_tickets):
    try:
        #assert()
        arrow_table = pq.read_table('preprocess.parquet')

        # Convert the Arrow Table back to a pandas DataFrame
        feature_vector_df = arrow_table.to_pandas()
    except:
        # ... (all preprocessing code from the original question without changes) ...
        error_list = df_original.groupby(['sid', 'memoryid']).agg({'error_type' : set})
        error_list = error_list.reset_index()

        filter_elem = category
        original_columns = df_original.columns
        filtered_df = pd.merge(df_original,filter_elem, on=['sid','memoryid'],how = 'inner', suffixes=('','_cat'))
        filtered_df = generate_min_max(filtered_df)
        #filtered_df = groupby_machine_informations(filtered_df)
        feature_vector_df = filtered_df.groupby(['sid', 'memoryid']).agg({'rankid': list, 'bankid': list, 'row': list, 'col': list, 'error_time_min': list, 'error_time_max': list})
        #feature_vector_df['DRAM_model'] = feature_vector_df['DRAM_model'].apply(lambda x: x[0])

        feature_vector_df = pd.merge(feature_vector_df,error_list, on=['sid','memoryid'],how = 'inner')
        feature_vector_df = pd.merge(feature_vector_df,filter_elem, on=['sid','memoryid'],how = 'inner')
        feature_vector_df = pd.merge(feature_vector_df,trouble_tickets, on=['sid'],how = 'left')
        feature_vector_df['bankid'] = feature_vector_df['bankid_x']
        feature_vector_df['rankid'] = feature_vector_df['rankid_x']
        feature_vector_df = groupby_machine_informations(feature_vector_df)
        feature_vector_df = feature_vector_df.reset_index(drop=True)
        feature_vector_df = feature_vector_df[(feature_vector_df['DRAM_model']!="B2") & (feature_vector_df['DRAM_model']!="B3")]
        feature_vector_df = feature_vector_df.reset_index(drop=True)

        feature_vector_df['uniq_rows'] = feature_vector_df['row'].apply(lambda x: len(set(x)))
        feature_vector_df['uniq_cols'] = feature_vector_df['col'].apply(lambda x: len(set(x)))
        feature_vector_df['uniq_banks'] = feature_vector_df['bankid'].apply(lambda x: len(set(x)))
        feature_vector_df['uniq_ranks'] = feature_vector_df['rankid'].apply(lambda x: len(set(x)))
        feature_vector_df['uniq_events'] = feature_vector_df['error_time_min'].apply(lambda x: len(set(x)))
        try:
            feature_vector_df['cluster'] = feature_vector_df['Classification_detail']
        except:
            feature_vector_df['cluster'] =feature_vector_df['category']
        feature_vector_df['failure_type'] = feature_vector_df['failure_type'].apply(
            lambda x: 'Uncorrectable' if x == 1 else 'Correctable' if x == 2 else 'System failure' if x == 3 else 'NoFailure')

        '''
            error_type is a set of 1,2,3
            From -> to
            1: read
            2: scrub
            3: write
            could have multiple values. Need to convert to a values
            when 1,2 are present, need to convert to read_scrub
            when 1,3 are present, need to convert to read_write
            ...
        '''
        feature_vector_df['error_type'] = feature_vector_df['error_type'].apply(
            lambda x: 'read' if x == {1} else 'scrub' if x == {2} else 'write' if x == {3} else 'read_scrub' if x == {1, 2} else 'read_write' if x == {1, 3} else 'scrub_write' if x == {2, 3} else 'read_scrub_write' if x == {1, 2, 3} else 'NoFailure')
        arrow_table = pa.Table.from_pandas(feature_vector_df)

        # Save the Arrow Table as a Parquet file
        pq.write_table(arrow_table, 'preprocess.parquet')



    msocket = feature_vector_df[(feature_vector_df['category'] == 'multi_socket' )| (feature_vector_df['category'] == 'bank_control') ]
    feature_vector_df = feature_vector_df[feature_vector_df['category'] != 'multi_socket'] 
    feature_vector_df = feature_vector_df[feature_vector_df['category'] != 'bank_control']

    _,msocket_class = make_decision(msocket,False,True)
    category_msocket = pd.DataFrame(columns=['sid', 'memoryid', 'rankid', 'bankid', 'category', 'permanency'])
    for key in msocket_class.keys():
        msocket_class
        if msocket_class[key].empty:
            continue
        category_msocket = pd.concat([category_msocket, pd.DataFrame({'sid': msocket_class[key]['sid'], 'memoryid': msocket_class[key]['memoryid'], 'category': key, 'permanency': 'permanent'})])

    # join category_msocket to msocket using sid, memoryid. 
    # from m socket, remove category, permanency, and add category_msocket's category and permanency
    msocket = msocket.drop(['category', 'permanency'], axis=1)
    category_msocket = category_msocket.drop(['rankid', 'bankid'], axis=1)
    msocket = pd.merge(msocket, category_msocket, on=['sid', 'memoryid'], how='left')
    msocket = msocket.reset_index(drop=True)
    feature_vector_df = pd.concat([feature_vector_df, msocket])
    feature_vector_df = feature_vector_df.reset_index(drop=True)


    return feature_vector_df
def plot_cluster_data(args):
    _category,cluster,DRAM_MODEL, feature_vector_df = args
    plot_cluster(cluster,DRAM_MODEL,feature_vector_df, _category)


def plot_cluster(cluster,DRAM_MODEL,feature_vector_df, _category):
    cluster_df = feature_vector_df[feature_vector_df['cluster'] == cluster]
    cluster_df = cluster_df[cluster_df['DRAM_model'] == DRAM_MODEL]
    print ("start:",DRAM_MODEL,cluster)
    for index, row in cluster_df.iterrows():
        # if file exists, then skip
        #if os.path.exists(f'kmeans/{_category}/cluster_{cluster}/{row["sid"]}_{row["memoryid"]}.png'):
        if os.path.exists(f'kmeans_no_msocket/{_category}/{cluster}/{row["DRAM_model"]}/{row["sid"]}_{row["memoryid"]}.png'):
            continue
        print(f'{_category}/{cluster}/{row["DRAM_model"]}/{row["sid"]}_{row["memoryid"]}.png')
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(18.5, 10.5))
        fig.suptitle('row, col, bank, rank, evnts, first_time, last_time\n {}, {}, {}, {}, {}, {}, {}\n{}   {}\nMachine_vender:{} {}'.format(
            row['uniq_rows'], row['uniq_cols'], row['uniq_banks'], row['uniq_ranks'], row['uniq_events'], min(row['error_time_min']), max(row['error_time_max']),row['failed_time'],row["error_type"],
            row["server_manufacturer"], row["failure_type"]))

        # draw a grid on y axis for every 2048 rows.
        ax1.yaxis.set_ticks(np.arange(0, max(row['row']) + 512, 2048))
        ax2.yaxis.set_ticks(np.arange(0, max(row['row']) + 512, 2048))

        ax1.yaxis.grid(linestyle='--', color='gray')
        ax2.yaxis.grid(linestyle='--', color='gray')


        for bank in set(row['bankid']):
            # filter the data
            bank_filter = np.array(row['bankid']) == bank
            row_filter = np.array(row['row'])
            col_filter = np.array(row['col'])
            
            ax1.scatter(col_filter[bank_filter & (np.array(row['rankid']) == 0)], row_filter[bank_filter & (np.array(row['rankid']) == 0)], label=f'bank {bank}')
            ax2.scatter(col_filter[bank_filter & (np.array(row['rankid']) == 1)], row_filter[bank_filter & (np.array(row['rankid']) == 1)], label=f'bank {bank}')
        
        # set plot limits
        xlim = [min(row['col']) - 8, max(row['col']) + 8]
        ylim = [min(row['row']) - 8, max(row['row']) + 8]
        ax1.set_xlim(xlim)
        ax1.set_ylim(ylim)
        ax2.set_xlim(xlim)
        ax2.set_ylim(ylim)

        # place the legend outside the plot on the bottom mid
        ax1.legend(loc='upper center', bbox_to_anchor=(0.5, -0.05), fancybox=True, shadow=True, ncol=5)
        ax2.legend(loc='upper center', bbox_to_anchor=(0.5, -0.05), fancybox=True, shadow=True, ncol=5)
        
        fig.savefig(f'kmeans_no_msocket/{_category}/{cluster}/{row["DRAM_model"]}/{row["sid"]}_{row["memoryid"]}.png')
        plt.close(fig)



# Load using pyarrow
try:
    #assert()
    arrow_table = pq.read_table('feature_vector_df.parquet')

    # Convert the Arrow Table back to a pandas DataFrame
    feature_vector_df = arrow_table.to_pandas()

except:
    # Load data
    df_original, category, trouble_tickets = load_and_merge_data()

    # Preprocess data
    feature_vector_df = preprocess_data(df_original, category, trouble_tickets)

    #use pyarrow to save the data
    arrow_table = pa.Table.from_pandas(feature_vector_df)

    # Save the Arrow Table as a Parquet file
    pq.write_table(arrow_table, 'feature_vector_df.parquet')



def get_combinations(feature_vector_df):
    categories = feature_vector_df['category'].unique()
    categories = categories[categories!= "multiple_single_bit_failures"]
    clusters = feature_vector_df['cluster'].unique()
    dram_models = feature_vector_df['DRAM_model'].unique()
    return list(itertools.product(categories, clusters, dram_models))

def perform_analysis(feature_vector_df):
    # Create combinations of categories, clusters, and DRAM_models
    combinations = get_combinations(feature_vector_df)

    # Filter feature_vector_df by category and cluster for each unique category-cluster pair
    category_cluster_filtered_dfs = {
        (category, cluster): feature_vector_df[(feature_vector_df['category'] == category) & (feature_vector_df['cluster'] == cluster)]
        for category, cluster in itertools.product(feature_vector_df['category'].unique(), feature_vector_df['cluster'].unique())
    }

    # Create a multiprocessing pool and map the plot_cluster_data function over the combinations
    with Pool(processes=16) as pool:
        pool.map(
            plot_cluster_data,
            [
                (
                    category,
                    cluster,
                    dram_model,
                    category_cluster_filtered_dfs[(category, cluster)]
                ) for category, cluster, dram_model in combinations
            ]
        )

categories = feature_vector_df['category'].unique()
for _category in categories:
    if _category == "multiple_single_bit_failures":
        continue
    feature_vector_df_internal = feature_vector_df[feature_vector_df['category'] == _category]
    # build a directory
    # if directory already exists, it will not create a new one
    base_dir = 'kmeans_no_msocket'
    category_col = 'category'
    cluster_col = 'cluster'
    dram_model_col = 'DRAM_model'

    categories = feature_vector_df_internal[category_col].unique()
    clusters = feature_vector_df_internal[cluster_col].unique()
    failure_types = feature_vector_df_internal['failure_type'].unique()
    dram_models = feature_vector_df_internal[dram_model_col].unique()
    #clusters = clusters.dropna()
    directories = [os.path.join(base_dir, category, cluster, dram_model)
                for category in categories
                for cluster in clusters
                for dram_model in dram_models]

    for directory in directories:
        if not os.path.exists(directory):
            os.makedirs(directory)


# Call the perform_analysis function
perform_analysis(feature_vector_df)


feature_vector_df[['sid',	'memoryid',	'error_type',
                   	'category',	'permanency',	'failure_type',
                        	'DRAM_model',	'server_manufacturer',	'cluster']].to_csv('feature_vector_df_nosocket.csv')

