import dask.dataframe as dd
import pandas as pd
import numpy as np
import pickle
import seaborn as sns
import matplotlib.pyplot as plt


#load dataframe
# datatype : sid,memoryid,rankid,bankid,row,col,error_type,error_time
def load_csv(filename):
    df = pd.read_csv(filename, dtype={'sid': 'str', 'memoryid': 'int64', 'rankid': 'int64', 'bankid': 'int64', 'row': 'int64', 'col': 'int64', 'error_type': 'int64', 'error_time': 'str'},nrows=100000)
    return df

def process_data(df):
    df = df.copy()
    df = df.groupby(['memoryid', 'rankid', 'bankid', 'row', 'col']).count()
    return df

df = load_csv('mcelog.csv')

'''
Now we know 0001-01-01 means 2019-10-01. convert the time
'''
df['error_time'] = df['error_time'].apply(lambda x: x.split('-'))
date = df['error_time'].apply(lambda x: x[0] + '-' + x[1] + '-' + x[2])
# convert date 0001-01-01 to 2019-10-01 0001-08-01 to 2020-05-01
date = date.apply(lambda x: x.replace('0001-01', '2019-10'))
date = date.apply(lambda x: x.replace('0001-02', '2019-11'))
date = date.apply(lambda x: x.replace('0001-03', '2019-12'))
date = date.apply(lambda x: x.replace('0001-04', '2020-01'))
date = date.apply(lambda x: x.replace('0001-05', '2020-02'))
date = date.apply(lambda x: x.replace('0001-06', '2020-03'))
date = date.apply(lambda x: x.replace('0001-07', '2020-04'))
date = date.apply(lambda x: x.replace('0001-08', '2020-05'))


# create new column for error features
# First, groupby memoryid, rankid, bankid, row, col and error_time in 24 hours
df['error_time'] = date
df['error_time'] = pd.to_datetime(df['error_time'])
df['error_time'] = df['error_time'].dt.floor('7D')
df = df.groupby(['sid','memoryid', 'rankid', 'bankid', 'row', 'col', 'error_time']).count()
#memory id == 0
df = df.reset_index()
# remove duplicated rows
df = df.drop_duplicates()
df = df.sort_values(['sid','memoryid','rankid','bankid','row','col','error_time'])

# Second, when sid, memoryid, errortime are the same, compute rankid, bankid, row, col difference
df['rankid_diff'] = df.groupby(['sid', 'memoryid', 'error_time'])['rankid'].diff()
df['bankid_diff'] = df.groupby(['sid', 'memoryid', 'error_time'])['bankid'].diff()
df['row_diff'] = df.groupby(['sid', 'memoryid', 'error_time'])['row'].diff()
df['col_diff'] = df.groupby(['sid', 'memoryid', 'error_time'])['col'].diff()

# For each time and server_id, generate diff feature vectors
feature_vector_df = df.groupby(['sid', 'memoryid', 'error_time']).agg({'rankid_diff': list, 'bankid_diff': list, 'row_diff': list, 'col_diff': list})
feature_vector_df = feature_vector_df.reset_index()

# store df in pickle
with open('feature_vector_df.pkl', 'wb') as f:
    pickle.dump(feature_vector_df, f)

with open('df.pkl', 'wb') as f:
    pickle.dump(df, f)

multi_rank_failures = feature_vector_df[feature_vector_df['rankid_diff'].apply(lambda x: [i for i in x if str(i) != 'nan']).apply(np.unique).apply(len)>1]
multi_bank_failures = feature_vector_df[feature_vector_df['bankid_diff'].apply(lambda x: [i for i in x if str(i) != 'nan']).apply(np.unique).apply(len)>1]

row_failures = feature_vector_df[feature_vector_df['row_diff'].apply(lambda x: [i for i in x if str(i) != 'nan']).apply(np.unique).apply(len)>1]
column_failures = feature_vector_df[feature_vector_df['col_diff'].apply(lambda x: [i for i in x if str(i) != 'nan']).apply(np.unique).apply(len)>1]

# bank failure is the intersection of row and column failures
bank_failures = row_failures[row_failures.index.isin(column_failures.index)]

# for real row/column failures, we need to remove the bank failures
row_failures = row_failures[~row_failures.index.isin(bank_failures.index)]
column_failures = column_failures[~column_failures.index.isin(bank_failures.index)]

# Generate heatmap of errors for each server and memory
# First, groupby sid, memoryid, row, col,bank_id, rank_id and count the number of errors
heat_df = df.groupby(['sid', 'memoryid', 'row', 'col', 'bankid', 'rankid','error_time']).count()
heat_df = heat_df.reset_index()

with open('heat_df.pkl', 'wb') as f:
    pickle.dump(heat_df, f)

# Store the error features in pickle
with open('multi_rank_failures.pkl', 'wb') as f:
    pickle.dump(multi_rank_failures, f)
with open('multi_bank_failures.pkl', 'wb') as f:
    pickle.dump(multi_bank_failures, f)
with open('row_failures.pkl', 'wb') as f:
    pickle.dump(row_failures, f)
with open('column_failures.pkl', 'wb') as f:
    pickle.dump(column_failures, f)
with open('bank_failures.pkl', 'wb') as f:
    pickle.dump(bank_failures, f)

# draw heatmap using heat_df

# draw heatmap for each server id and error time
# find unique sid
sid = heat_df['sid'].unique()
for i in range(len(sid)):
    # find unique error time
    error_time = heat_df[heat_df['sid'] == sid[i]]['error_time'].unique()
    for j in range(len(error_time)):
        # filter sid and error time
        temp_df = heat_df[(heat_df['sid'] == sid[i]) & (heat_df['error_time'] == error_time[j])]
        # draw heatmap
        temp_df = temp_df.pivot(index='row', columns='col', values='error_type')
        
        temp_df = temp_df.reindex(index=np.arange(temp_df.index.min(), temp_df.index.max() + (temp_df.index.max() -temp_df.index.min() )//100+1,((temp_df.index.max() -temp_df.index.min() )//100)+1))
        # col should be contiguous with granularity 8
        temp_df = temp_df.reindex(columns=np.arange(temp_df.columns.min(), temp_df.columns.max() + (temp_df.columns.max() -temp_df.columns.min() )//100+1,((temp_df.columns.max() -temp_df.columns.min() )//100)+1))
        
        sns.heatmap(temp_df, cmap="YlGnBu")
        plt.title('sid: {} error_time: {}'.format(sid[i], error_time[j]))
        plt.show()
        pass