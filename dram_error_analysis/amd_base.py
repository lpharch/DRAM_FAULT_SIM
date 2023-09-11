# AMD approch based analysis
'''
Assumption : All dram would experience only one types of failure during measurement.
Then, if the number of bits is larger than 5, that is the real multi bit failure. Otherwise, it is multiple single bit failures.
'''

from array import array
from doctest import REPORT_CDIFF
import dask.dataframe as dd
import pandas as pd
import numpy as np
import pickle
import seaborn as sns
import matplotlib.pyplot as plt
import dask
import dask.diagnostics


from utils import groupby_machine_informations
from utils import make_decision
from utils import conv_mapping
from utils import line_analysis

from dask.diagnostics import ProgressBar
import matplotlib.cm as cm


#load dataframe
# datatype : sid,memoryid,rankid,bank
# id,row,col,error_type,error_time
def load_csv(filename):
    df = pd.read_csv(filename, dtype={'sid': 'str', 'memoryid': 'int64', 'rankid': 'int64', 'bankid': 'int64', 'row': 'int64', 'col': 'int64', 'error_type': 'int64', 'error_time': 'str'})
    return df

def process_data(df):
    df = df.copy()
    df = df.groupby(['memoryid', 'rankid', 'bankid', 'row', 'col']).count()
    return df

# find unique row, col 
def find_unique_failures(failures,group_by,aggregate_by, agg_func):
    failures.reset_index(inplace=True)
    days = []
    for d in range(1,7):
        days.append(failures.groupby(group_by)[aggregate_by].shift(-1) != (failures[aggregate_by] + pd.DateOffset(d)))

    s = pd.concat(days, axis=1)
    s = s.all(axis=1)
    s = s & (failures.groupby(group_by+['row'])[aggregate_by].shift(-1) != (failures[aggregate_by]))
    s = s & (failures.groupby(group_by+['col'])[aggregate_by].shift(-1) != (failures[aggregate_by]))
    s = s & (failures.groupby(group_by+['bankid'])[aggregate_by].shift(-1) != (failures[aggregate_by]))
    failures['temp'] = failures.loc[s,aggregate_by]
    failures['temp'] = failures.groupby(group_by)['temp'].fillna(method='backfill')
    return failures

def draw_scattor_plot(df, name, samples,xrange=None,yrange=None):
    data = df.apply(lambda x: list(zip(x['bankid'], x['rankid'], x['row'], x['col'])), axis=1)
    data = pd.DataFrame(data)
    data['sid'] = df['sid']
    if data.shape[0] > samples:
        data = data.sample(samples)
    if data.size == 0:
        return
    data.reset_index(inplace=True,drop=True)
    _server = data['sid']
    data = data[0]    
    # figure has subplots of 16 banks and 2 ranks
    fig, axes = plt.subplots(16, 2, figsize=(55, 220))
    fig.suptitle(name + ' Failure', fontsize=20)
    for i in range(data.size):
        for j in range(16):
            for k in range(2):
                axes[j, k].scatter([x[3] for x in data[i] if x[0] == j and x[1] == k], [x[2] for x in data[i] if x[0] == j and x[1] == k])
                axes[j, k].set_title('Bank: ' + str(j) + ' Rank: ' + str(k), fontsize=38)
                axes[j, k].set_xlabel('Column', fontsize=38)
                axes[j, k].set_ylabel('Row', fontsize=38) 
                axes[j, k].tick_params(axis='both', which='major', labelsize=38)
                
                # set x and y axis limit
                if xrange is not None:
                    axes[j, k].set_xlim(xrange[0],xrange[1])
                else:
                    axes[j, k].set_xlim(0, 2**10)
                if yrange is not None:
                    axes[j, k].set_ylim(yrange[0], yrange[1])
                else:
                    axes[j, k].set_ylim(0, 2**17)

    #set legends
    plt.savefig('/home/jeageunli/Desktop/output/' + name + '.png')
    plt.close()

def draw_scattor_plot_results(df, name, samples,xrange=None,yrange=None):
    data = df.apply(lambda x: list(zip(x['bankid'], x['rankid'], x['row'], x['col'])), axis=1)
    data = pd.DataFrame(data)
    data['sid'] = df['sid']
    if samples == 1:
        idx = data.apply(lambda x: len(x.explode()),axis = 1).argmax()
        data = data.iloc[idx:idx+1]
    if data.shape[0] > samples:
        data = data.sample(samples)
    if data.size == 0:
        return
    data.reset_index(inplace=True,drop=True)
    _server = data['sid']
    data = data[0]    
    # figure has subplots of 16 banks and 2 ranks
    fig, axes = plt.subplots(16, 2, figsize=(55, 220))
    fig.suptitle(name + ' Failure', fontsize=20)
    for i in range(data.size):
        for j in range(16):
            for k in range(2):
                axes[j, k].scatter([x[3] for x in data[i] if x[0] == j and x[1] == k], [x[2] for x in data[i] if x[0] == j and x[1] == k])
                axes[j, k].set_title('Bank: ' + str(j) + ' Rank: ' + str(k), fontsize=38)
                axes[j, k].set_xlabel('Column', fontsize=38)
                axes[j, k].set_ylabel('Row', fontsize=38) 
                axes[j, k].tick_params(axis='both', which='major', labelsize=38)
                
                # set x and y axis limit
                if xrange is not None:
                    axes[j, k].set_xlim(xrange[0],xrange[1])
                else:
                    axes[j, k].set_xlim(0, 2**10)
                if yrange is not None:
                    axes[j, k].set_ylim(yrange[0], yrange[1])
                else:
                    axes[j, k].set_ylim(0, 2**17)

    #set legends
    plt.savefig('/home/jeageunli/Desktop/output/' + name + '.png')
    plt.close()
    

#draw multiple sid
def draw_scattor_plot_multiple_sid(df, name, samples,xrange=None,yrange=None):
    # input df has multiple sid, and for each sid, it has multiple failure
    # for the first graph, draw for sid 0 of memoryid 0 for the second graph, draw for sid 0 of memoryid 1
    data = df.apply(lambda x: list(zip(x['bankid'], x['rankid'], x['row'], x['col'])), axis=1)
    data = pd.DataFrame(data)
    data['sid'] = df['sid']
    group_order = np.random.permutation(df['sid'].unique())
    data['group_shuffled'] = pd.Categorical(df['sid'], categories=group_order, ordered=True)

    # sort the DataFrame by the new column
    data = data.sort_values('group_shuffled').reset_index(drop=True)
    data.drop('group_shuffled', axis=1, inplace=True)


    if data.size == 0:
        return
    data.reset_index(inplace=True,drop=True)

    _serverid = data.groupby('sid').cumcount()
   
    data = data[0]    

    # figure has subplots of 8 unique memoryid 
    fig, axes = plt.subplots(4, 2, figsize=(55, 88))
    fig.suptitle(name + ' Failure', fontsize=20)
    count = 0
    for i in range(data.size):
        # use the same color for each loop but it needs to repeat after 32 times  
        for j in range(4):
            for k in range(2):
                
                module_idx = j*2 + k
                axes[j, k].scatter(
                    [x[3] for x in data[i] if _serverid[i] == module_idx],
                     [x[2] for x in data[i]  if _serverid[i] == module_idx])
                axes[j, k].set_title('module: ' + str(module_idx), fontsize=38)
                axes[j, k].set_xlabel('Column', fontsize=38)
                axes[j, k].set_ylabel('Row', fontsize=38) 
                axes[j, k].tick_params(axis='both', which='major', labelsize=38)
                
                # set x and y axis limit
                if xrange is not None:
                    axes[j, k].set_xlim(xrange[0],xrange[1])
                else:
                    axes[j, k].set_xlim(0, 2**10)
                if yrange is not None:
                    axes[j, k].set_ylim(yrange[0], yrange[1])
                else:
                    axes[j, k].set_ylim(0, 2**17)
        count += 1
        if count > samples:
            break
    
    plt.savefig('/home/jeageunli/Desktop/output/' + name + '.png')
    plt.close()


if __name__ == '__main__':

    df = load_csv('mcelog.csv')
    #df = df.sort_values(by=['error_time'])

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
    df['error_time'] = dd.to_datetime(df['error_time'])

    # if there exist a file data_df.pkl or data_df.json, load it instead of processing data   

    try:
        with open('data_df.pkl', 'rb') as f:
            df = pickle.load(f)
    except: 
        df = df.compute()
        # write data_df.pkl
        with open('data_df.pkl', 'wb') as f:
            pickle.dump(df, f)


        

    # process data
    #group by each memoryid, rankid, bankid, row, col and error_time
    df.reset_index(inplace=True)
    df = df.sort_values(by=['sid', 'memoryid', 'rankid', 'bankid', 'row', 'col','error_time'])
    df = df.reset_index(drop=True)


    min_df = df.groupby(['sid','memoryid', 'rankid', 'bankid', 'row', 'col']).min()
    max_df = df.groupby(['sid','memoryid', 'rankid', 'bankid', 'row', 'col']).max()
    df = pd.merge(min_df, max_df, on=['sid','memoryid', 'rankid', 'bankid', 'row', 'col'],suffixes=('_min', '_max'))
    df = df[['error_time_min','error_time_max']]    
    df = df.reset_index()

    df = groupby_machine_informations(df)
    backup_df = df.copy()

    df.reset_index(inplace=True, drop=True)
    feature_vector_df = df.groupby(['sid', 'memoryid']).agg({'rankid': list, 'bankid': list, 'row': list, 'col': list, 'error_time_min': list, 'error_time_max': list, 'DRAM_model': list})
    feature_vector_df['DRAM_model'] = feature_vector_df['DRAM_model'].apply(lambda x: x[0])
    feature_vector_df.reset_index(inplace=True)
    #feature_vector_df = feature_vector_df[(feature_vector_df['DRAM_model']!="B2") & (feature_vector_df['DRAM_model']!="B3")]
    feature_vector_df = feature_vector_df[(feature_vector_df['DRAM_model']=="B3")]
    log_res, phy_res = make_decision(feature_vector_df)

    '''
    1. If single row error => which row has errors? histogram
    2. If single column error => which column has errors? histogram

    3. If single bank error => which row, column has errors? histogram
    '''
    if False:
        plt.cla()
        
        # single row error histogram
        single_row_error = single_row['row'].apply(lambda x:x[0])
        single_row_error.hist(bins=200,figsize=(20,10))
        plt.savefig('/home/jeageunli/Desktop/single_row_hist_'+model_name+'.png')
        plt.cla()

        # single column error histogram
        single_column_error = single_column['col'].apply(lambda x:x[0])
        single_column_error.hist(bins=200)
        plt.savefig('/home/jeageunli/Desktop/single_column_hist_'+model_name+'.png')
        plt.cla()

        # single bank error histogram
        single_bank_row_hist = single_bank['row'].apply(lambda x:set((np.array(x)//1)*1))
        single_bank_row_hist = single_bank_row_hist.explode()
        single_bank_row_hist.hist(bins=512,figsize=(30,10))
        plt.savefig('/home/jeageunli/Desktop/single_bank_row_hist_'+model_name+'.png')
        plt.cla()

        single_bank_column_hist = single_bank['col'].apply(lambda x:set((np.array(x)//1)*1))
        single_bank_column_hist = single_bank_column_hist.explode()
        single_bank_column_hist.hist(bins=128,figsize=(30,10))
        plt.savefig('/home/jeageunli/Desktop/single_bank_column_hist_'+model_name+'.png')
        plt.cla()

        # multi bank error histogram
        multi_bank_row_hist = multi_bank['row'].apply(lambda x:set((np.array(x)//1)*1))
        multi_bank_row_hist = multi_bank_row_hist.explode()
        multi_bank_row_hist.hist(bins=512,figsize=(30,10))
        plt.savefig('/home/jeageunli/Desktop/multi_bank_row_hist_'+model_name+'.png')
        plt.cla()

        multi_bank_column_hist = multi_bank['col'].apply(lambda x:set((np.array(x)//1)*1))
        multi_bank_column_hist = multi_bank_column_hist.explode()
        multi_bank_column_hist.hist(bins=128,figsize=(30,10))
        plt.savefig('/home/jeageunli/Desktop/multi_bank_column_hist_'+model_name+'.png')
        plt.cla()

        # multi rank error histogram
        multi_rank_row_hist = multi_rank['row'].apply(lambda x:set((np.array(x)//1)*1))
        multi_rank_row_hist = multi_rank_row_hist.explode()
        multi_rank_row_hist.hist(bins=512,figsize=(30,10))
        plt.savefig('/home/jeageunli/Desktop/multi_rank_row_hist_'+model_name+'.png')
        plt.cla()

        multi_rank_column_hist = multi_rank['col'].apply(lambda x:set((np.array(x)//1)*1))
        multi_rank_column_hist = multi_rank_column_hist.explode()
        multi_rank_column_hist.hist(bins=128,figsize=(30,10))
        plt.savefig('/home/jeageunli/Desktop/multi_rank_column_hist_'+model_name+'.png')
        plt.cla()



    # draw figures per group
    for key in log_res.keys():
        exec(f"{key} = log_res[key]")

    for key in phy_res.keys():
        exec(f"{key} = phy_res[key]")

    for key in phy_res.keys():
        for model in ['A1','B1','C1','A2','C2']:
            exec(f"{key}_{model} = phy_res[key][phy_res[key]['DRAM_model']==model]")
    
    

    draw_scattor_plot(local_wordline,"local_wordline",1)
    draw_scattor_plot(single_sense_amp,"single_sense_amp",1)
    draw_scattor_plot(decoder_single_col,"decoder_single_col",1)
    draw_scattor_plot(single_csl_column,"single_csl_column",1)
    draw_scattor_plot(not_clustered_single_column,"not_clustered_single_column",1)
    draw_scattor_plot(subarray_row_decoder,"subarray_row_decoder",1)
    draw_scattor_plot(subarray_row_decoder_two_clusters,"subarray_row_decoder_two_clusters",1)
    draw_scattor_plot(multi_csls,"multi_csls",1)
    draw_scattor_plot(lwl_sel,"lwl_sel",1)
    draw_scattor_plot(lwl_sel2,"lwl_sel2",1)
    draw_scattor_plot(decoder_multi_col,"decoder_multi_col",1)
    draw_scattor_plot(not_clustered_single_bank,"not_clustered_single_bank",1)
    draw_scattor_plot(single_csl_bank,"single_csl_bank",1)
    draw_scattor_plot(bank_control,"bank_control",1)
    draw_scattor_plot(row_addr_mux,"row_addr_mux",1)
    draw_scattor_plot(not_clustered_multi_bank,"not_clustered_multi_bank",1)
    draw_scattor_plot(global_row_decoder_two_clusters,"global_row_decoder_two_clusters",1)


    for model_name in ["A1","A2","A3","B1","B2","B3","C1","C2","C3"]:
        print("local_wordline_"+model_name,local_wordline[local_wordline.DRAM_model==model_name].shape[0])
        print("single_sense_amp_"+model_name,single_sense_amp[single_sense_amp.DRAM_model==model_name].shape[0])
        print("decoder_single_col_"+model_name,decoder_single_col[decoder_single_col.DRAM_model==model_name].shape[0])
        print("single_csl_column_"+model_name,single_csl_column[single_csl_column.DRAM_model==model_name].shape[0])
        print("not_clustered_single_column_"+model_name,not_clustered_single_column[not_clustered_single_column.DRAM_model==model_name].shape[0])
        print("subarray_row_decoder_"+model_name,subarray_row_decoder[subarray_row_decoder.DRAM_model==model_name].shape[0])
        print("subarray_row_decoder_two_clusters_"+model_name,subarray_row_decoder_two_clusters[subarray_row_decoder_two_clusters.DRAM_model==model_name].shape[0])
        print("multi_csls_"+model_name,multi_csls[multi_csls.DRAM_model==model_name].shape[0])
        print("lwl_sel_"+model_name,lwl_sel[lwl_sel.DRAM_model==model_name].shape[0])
        print("lwl_sel2_"+model_name,lwl_sel2[lwl_sel2.DRAM_model==model_name].shape[0])
        print("decoder_multi_col_"+model_name,decoder_multi_col[decoder_multi_col.DRAM_model==model_name].shape[0])
        print("not_clustered_single_bank_"+model_name,not_clustered_single_bank[not_clustered_single_bank.DRAM_model==model_name].shape[0])
        print("single_csl_bank_"+model_name,single_csl_bank[single_csl_bank.DRAM_model==model_name].shape[0])
        print("bank_control_"+model_name,bank_control[bank_control.DRAM_model==model_name].shape[0])
        print("row_addr_mux_"+model_name,row_addr_mux[row_addr_mux.DRAM_model==model_name].shape[0])
        print("not_clustered_multi_bank_"+model_name,not_clustered_multi_bank[not_clustered_multi_bank.DRAM_model==model_name].shape[0])
        print("global_row_decoder_two_clusters_"+model_name,global_row_decoder_two_clusters[global_row_decoder_two_clusters.DRAM_model==model_name].shape[0])
    #copmare row within the same sid,memoryid
    
    for model_name in ["A1","A2","A3","B1","B2","B3","C1","C2","C3"]:
        print("single_row_"+model_name,single_row[single_row.DRAM_model==model_name].shape[0])
        print("single_column_"+model_name,single_column[single_column.DRAM_model==model_name].shape[0])
        print("single_bank_"+model_name,single_bank[single_bank.DRAM_model==model_name].shape[0])
        print("multi_bank_"+model_name,multi_bank[multi_bank.DRAM_model==model_name].shape[0])
        print("multiple_single_bit_failures_"+model_name,multiple_single_bit_failures[multiple_single_bit_failures.DRAM_model==model_name].shape[0])

    #is numpy array empty

    # single column
    '''
    4k * 1k * 8 = 32MB ==> 40 * 8 320 errors ==> 10% -> 1%
    Rank ecc proposed 1bit error correction vs BG ecc proposed 1 bit error corection

    
    0.066 FIT /1Mbit was what they reported for error    
    

        # per failure type, and per failure, plot 
        fig = plt.figure(figsize=(20, 10))
        single_column.row.apply(np.unique).apply(len).hist(bins=256,log=True)
        plt.title('single_column_failure_affected rowcount', fontsize=20)
        plt.xlabel('Affected row count', fontsize=20)
        plt.ylabel('Frequency(log)', fontsize=20)
        plt.ylim(0, 2100)        
        plt.savefig('single_column_rowcount.png')
        #set limit for y 
        

        fig = plt.figure(figsize=(20, 10))
        single_row.col.apply(np.unique).apply(len).hist(bins=128,log=True)
        plt.title('single_row_failure_affected column', fontsize=20)
        plt.xlabel('Affected column count', fontsize=20)
        plt.ylabel('Frequency(log)', fontsize=20)
        plt.ylim(0, 2100)
        plt.savefig('single_row_colcount.png')

        fig = plt.figure(figsize=(20, 10))
        single_bank.col.apply(np.unique).apply(len).hist(bins=128,log=True)
        plt.title('single_bank_failure_affected column', fontsize=20)
        plt.xlabel('Affected column count', fontsize=20)
        plt.ylabel('Frequency(log)', fontsize=20)
        plt.ylim(0, 2100)
        plt.savefig('single_bank_colcount.png')

        # per failure type, and per failure, plot 
        fig = plt.figure(figsize=(20, 10))
        single_bank.row.apply(np.unique).apply(len).hist(bins=256,log=True)
        plt.title('single_bank_failure_affected rowcount', fontsize=20)
        plt.xlabel('Affected row count', fontsize=20)
        plt.ylabel('Frequency(log)', fontsize=20)
        plt.ylim(0, 2100)
        plt.savefig('single_bank_rowcount.png')

        fig = plt.figure(figsize=(20, 10))
        multi_bank.col.apply(np.unique).apply(len).hist(bins=128,log=True)
        plt.title('multi_bank_failure_affected column', fontsize=20)
        plt.xlabel('Affected column count', fontsize=20)
        plt.ylabel('Frequency(log)', fontsize=20)
        plt.ylim(0, 2100)
        plt.savefig('multi_bank_colcount.png')

        fig = plt.figure(figsize=(20, 10))
        multi_bank.row.apply(np.unique).apply(len).hist(bins=256,log=True,range=(0,16*1024))
        plt.title('multi_bank_failure_affected rowcount', fontsize=20)
        plt.xlabel('Affected row count', fontsize=20)
        plt.ylabel('Frequency(log)', fontsize=20)
        plt.ylim(0, 2100)
        plt.xlim(0,16*1024)
        plt.savefig('multi_bank_rowcount.png')

        fig = plt.figure(figsize=(20, 10))
        multi_rank.col.apply(np.unique).apply(len).hist(bins=128,log=True)
        plt.title('multi_rank_failure_affected column', fontsize=20)
        plt.xlabel('Affected column count', fontsize=20)
        plt.ylabel('Frequency(log)', fontsize=20)
        plt.ylim(0, 2100)
        plt.savefig('multi_rank_colcount.png')

        fig = plt.figure(figsize=(20, 10))
        multi_rank.row.apply(np.unique).apply(len).hist(bins=256,log=True,range=(0,16*1024))
        plt.title('multi_rank_failure_affected rowcount', fontsize=20)
        plt.xlabel('Affected row count', fontsize=20)
        plt.ylabel('Frequency(log)', fontsize=20)
        plt.ylim(0, 2100)
        plt.xlim(0,16*1024)
        plt.savefig('multi_rank_rowcount.png')
    '''



    #concat single_bank and single_column
    test = pd.concat([single_bank,single_column])

    #at most two columns
    test = test[test.col.apply(np.unique).apply(len)<=2]

    #Count how many chunk of 1k are affected
    test = test[test.row.apply(np.unique).apply(lambda x:max(x)-min(x))>2048]
    testout = test.row.apply(np.unique).apply(lambda x:(np.array(x)-min(x))//1024).apply(np.unique).apply(lambda x:len(x))

    # join test.DRAM_model with testout
    testout = pd.concat([test.DRAM_model,testout],axis=1)

    # use only a first character of dram model
    testout.DRAM_model = testout.DRAM_model.apply(lambda x:x[0])

    testout = testout[testout.iloc[:,1]>2]

    # plot a histogram of testout font size 20
    #plot 2x2 figure with subfigure
    fig, axes = plt.subplots(nrows=2, ncols=2,figsize=(20,10))

    # set subfigure title and x,y axis label 
    axes[0,0].set_title('A', fontsize=16)   
    axes[0,0].set_xlabel('Affected subarray count', fontsize=14)
    axes[0,0].set_ylabel('Frequency(log)', fontsize=14)
    axes[0,0].hist(testout[testout.DRAM_model=='A'].iloc[:,1],bins=32, range=(0,32),log=True)
    axes[0,1].set_title('B', fontsize=16)   
    axes[0,1].set_xlabel('Affected subarray count', fontsize=14)
    axes[0,1].set_ylabel('Frequency(log)', fontsize=14)
    axes[0,1].hist(testout[testout.DRAM_model=='B'].iloc[:,1],bins=32, range=(0,32),log=True)
    axes[1,0].set_title('C', fontsize=16)   
    axes[1,0].set_xlabel('Affected subarray count', fontsize=14)
    axes[1,0].set_ylabel('Frequency(log)', fontsize=14)
    axes[1,0].hist(testout[testout.DRAM_model=='C'].iloc[:,1],bins=32, range=(0,32),log=True)

    # increase gap between subfigures
    fig.subplots_adjust(hspace=0.35)



    #remove the last subfigure
    fig.delaxes(axes[1,1])

    plt.savefig('/home/jeageunli/single_bank_rowcount.pdf')

    # Difference between the maximum and minimum row address in a single column error
    single_column[single_column.row.apply(np.unique).apply(lambda x:max(x)-min(x))==2]

    