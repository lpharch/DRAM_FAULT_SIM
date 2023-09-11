#visualize the data to file format.
import pandas as pd
import numpy as np
import pickle
import plotly.graph_objs as go
import matplotlib.pyplot as plt


from sklearn.preprocessing import StandardScaler
from utils import generate_min_max
from utils import groupby_machine_informations

#load categorical data
category = pickle.load(open('category.pkl', 'rb'))
category.memoryid = category.memoryid.astype('int64')

#load data
with open('data_df.pkl', 'rb') as f:
    df_original = pickle.load(f)
#df_original = df_original[:100000]

with open('trouble_tickets.pkl', 'rb') as f:
    trouble_tickets = pickle.load(f)

error_list = df_original.groupby(['sid', 'memoryid']).agg({'error_type' : set})
error_list = error_list.reset_index()

flag = False
for _category in np.unique(category.category):
    if _category == 'multiple_single_bit_failures':
        continue

        
    filter_elem = category[category['category'] == _category]
    filter_elem.memoryid = filter_elem.memoryid.astype('int64')
    filter_elem.reset_index(drop=True, inplace=True)
    filter_elem.dropna(inplace=True,axis=1)

    # filter using filter_elem
    original_columns = df_original.columns
    filtered_df = pd.merge(df_original,filter_elem, on=['sid','memoryid'],how = 'inner', suffixes=('','_cat'))

    # Using filtered_df extract some feature data;
    # 1. number of unique rows
    # 2. number of unique columns
    # 3. number of unique banks
    # 4. number of evnts
    # 5. Difference between max and min time

    filtered_df = generate_min_max(filtered_df)
    filtered_df = groupby_machine_informations(filtered_df)

    filtered_df = filtered_df.reset_index(drop=True)
    feature_vector_df = filtered_df.groupby(['sid', 'memoryid']).agg({'rankid': list, 'bankid': list, 'row': list, 'col': list, 'error_time_min': list, 'error_time_max': list, 'DRAM_model': list})
    feature_vector_df['DRAM_model'] = feature_vector_df['DRAM_model'].apply(lambda x: x[0])

    feature_vector_df = pd.merge(feature_vector_df,error_list, on=['sid','memoryid'],how = 'inner')
    feature_vector_df = pd.merge(feature_vector_df,filter_elem, on=['sid','memoryid'],how = 'inner')
    feature_vector_df = pd.merge(feature_vector_df,trouble_tickets, on=['sid'],how = 'left')
    feature_vector_df = feature_vector_df.reset_index(drop=True)
    feature_vector_df = feature_vector_df[(feature_vector_df['DRAM_model']!="B2") & (feature_vector_df['DRAM_model']!="B3")]
    feature_vector_df = feature_vector_df.reset_index(drop=True)

    feature_vector_df['uniq_rows'] = feature_vector_df['row'].apply(lambda x: len(set(x)))
    feature_vector_df['uniq_cols'] = feature_vector_df['col'].apply(lambda x: len(set(x)))
    feature_vector_df['uniq_banks'] = feature_vector_df['bankid'].apply(lambda x: len(set(x)))
    feature_vector_df['uniq_ranks'] = feature_vector_df['rankid'].apply(lambda x: len(set(x)))
    feature_vector_df['uniq_events'] = feature_vector_df['error_time_min'].apply(lambda x: len(set(x)))

    feature_vector_df['cluster'] = feature_vector_df['category']

    # Change the name for errortype and failuretype
    '''
    failure_type
    From -> to
    1: Uncorrectable
    2: Correctable
    3: System failure
    Nan: Unknown
    '''
    feature_vector_df['failure_type'] = feature_vector_df['failure_type'].apply(
        lambda x: 'Uncorrectable' if x == 1 else 'Correctable' if x == 2 else 'System failure' if x == 3 else 'Unknown')

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
        lambda x: 'read' if x == {1} else 'scrub' if x == {2} else 'write' if x == {3} else 'read_scrub' if x == {1, 2} else 'read_write' if x == {1, 3} else 'scrub_write' if x == {2, 3} else 'read_scrub_write' if x == {1, 2, 3} else 'Unknown')
        





    '''
    Build a directory as follows
    kmeans
    |---
        category_name
            |
            |--- cluster_0
            |--- cluster_1
            |--- cluster_2
            ...
            |--- cluster_k

    For each cluster, we will have a files with graph, which name is the sid and memoryid
    Each graph will have a plot, for each rank, it will have a scatter plot of x-aixs as column, and y-axis as row 
    '''
    import os
    # build a directory
    # if directory already exists, it will not create a new one
    base_dir = 'kmeans'
    category_col = 'category'
    cluster_col = 'cluster'
    dram_model_col = 'DRAM_model'

    categories = feature_vector_df[category_col].unique()
    clusters = feature_vector_df[cluster_col].unique()
    failure_types = feature_vector_df['failure_type'].unique()
    dram_models = feature_vector_df[dram_model_col].unique()

    directories = [os.path.join(base_dir, category, cluster, dram_model)
                for category in categories
                for cluster in clusters
                for dram_model in dram_models]

    for directory in directories:
        if not os.path.exists(directory):
            os.makedirs(directory)


    def plot_cluster(cluster,DRAM_MODEL,feature_vector_df, _category):
        cluster_df = feature_vector_df[feature_vector_df['cluster'] == cluster]
        cluster_df = cluster_df[cluster_df['DRAM_model'] == DRAM_MODEL]
        for index, row in cluster_df.iterrows():
            # if file exists, then skip
            #if os.path.exists(f'kmeans/{_category}/cluster_{cluster}/{row["sid"]}_{row["memoryid"]}.png'):
            if os.path.exists(f'kmeans/{_category}/{cluster}/{row["DRAM_model"]}/{row["sid"]}_{row["memoryid"]}.png'):
                continue
            
            fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(18.5, 10.5))
            fig.suptitle('row, col, bank, rank, evnts, first_time, last_time\n {}, {}, {}, {}, {}, {}, {}\n{}   {}'.format(
                row['uniq_rows'], row['uniq_cols'], row['uniq_banks'], row['uniq_ranks'], row['uniq_events'], min(row['error_time_min']), max(row['error_time_max']),row['failed_time'],row["error_type"]))

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
            ylim = [min(row['row']) - 512, max(row['row']) + 512]
            ax1.set_xlim(xlim)
            ax1.set_ylim(ylim)
            ax2.set_xlim(xlim)
            ax2.set_ylim(ylim)

            # place the legend outside the plot on the bottom mid
            ax1.legend(loc='upper center', bbox_to_anchor=(0.5, -0.05), fancybox=True, shadow=True, ncol=5)
            ax2.legend(loc='upper center', bbox_to_anchor=(0.5, -0.05), fancybox=True, shadow=True, ncol=5)
            
            fig.savefig(f'kmeans/{_category}/{cluster}/{row["DRAM_model"]}/{row["sid"]}_{row["memoryid"]}.png')
            plt.close(fig)


    from joblib import Parallel, delayed

    # for each cluster, we will have a files with graph, which name is the sid and memoryid
    Parallel(n_jobs=16)(delayed(plot_cluster)(cluster, dram_model, feature_vector_df, _category) for cluster in feature_vector_df.cluster.unique() for dram_model in feature_vector_df.DRAM_model.unique())



