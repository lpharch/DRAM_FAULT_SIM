import numpy as np
import pandas as pd
import pickle
import os
import matplotlib.pyplot as plt 
import itertools
import dask
import dask.dataframe as dd
import dask.diagnostics
import swifter

def generate_min_max(df):
    df = df.groupby(['sid','memoryid', 'rankid', 'bankid', 'row', 'col'])['error_time'].agg(['min', 'max', 'count'])
    df = df.rename(columns={'min': 'error_time_min', 'max': 'error_time_max'})    
    df = df.reset_index()
    return df

def line_analysis_col(df):
    ax = plt.gca()
    df.col.apply(np.sort).apply(lambda x: pd.DataFrame(x).rank(method='dense').values.flatten()).explode().reset_index().groupby('index').col.plot()
    for i in range (9):
        ax.axhline(y=i*16, color='k', linestyle='--',alpha=0.3)
    for i in range (5):
        ax.axhline(y=i*32, color='k', linestyle='--',alpha=0.3)
    for i in range(3):
        ax.axhline(y=i*64, color='k', linestyle='--',alpha=0.3) 


def line_analysis(df):
    ax = plt.gca()
    df.row.apply(lambda x: np.array(x)).apply(np.sort).explode().reset_index().groupby('index').row.plot()
    for i in range (33):
        ax.axhline(y=i*4*1024, color='k', linestyle='--',alpha=0.3)
    for i in range (17):
        ax.axhline(y=i*8*1024, color='k', linestyle='--',alpha=0.3)
    for i in range(9):
        ax.axhline(y=i*16*1024, color='k', linestyle='--',alpha=0.3) 


def make_tuple(a,b):
    return (a,b)

def custom_func(x,func):
    combinations = list(itertools.combinations(x, 2))
    return pd.DataFrame(combinations, columns=['value1', 'value2']).apply(lambda x: func(x['value1'],x['value2']), axis=1)


#using time intersection, if the there exist common time interval, then they are in the multi_socket failure             
def find_multisocket(df):
    multi_socket = df
    multi_socket.reset_index(inplace=True,drop=True)
    multi_socket['error_time_min'] = multi_socket['error_time_min'].swifter.apply(lambda x: min(x))
    multi_socket['error_time_max'] = multi_socket['error_time_max'].swifter.apply(lambda x: max(x))
    latest_start = multi_socket.groupby(['sid']).error_time_min.apply(lambda x: custom_func(x,max))
    earliest_end = multi_socket.groupby(['sid']).error_time_max.apply(lambda x: custom_func(x,min))
    intersections =  latest_start<=earliest_end
    iterations = multi_socket.groupby(['sid']).memoryid.apply(lambda x: custom_func(x,make_tuple))
    intersections = intersections.reset_index()
    iterations = iterations.reset_index()

    iterations = pd.merge(iterations,intersections, on=['sid','level_1'], how='inner')
    iterations = iterations[iterations[0]==True]
    iterations.reset_index(inplace=True,drop=True)
    # using iterations make sid, memoryid table. memoryid is tuple but need to be element by expand
    iterations = iterations[['sid','memoryid']].explode('memoryid').reset_index(drop=True)
    iterations = iterations[['sid','memoryid']].drop_duplicates()
    # keep multisocket only if sid and memoryid are in iterations
    multi_socket = multi_socket.join(iterations.set_index(['sid','memoryid']), on=['sid','memoryid'], how='inner')
    return multi_socket.reset_index(drop=True)



def make_decision(feature_vector_df,msocket = True, mrank=True):
    testdf = feature_vector_df.swifter.apply(lambda x: list(zip(x['row'], x['col'],x['rankid'],x['bankid'])), axis=1)
    testdf = testdf.swifter.apply(lambda x: set(x)).apply(len)

    multiple_single_bit_failures = feature_vector_df[testdf <= 2]
    multi_bit_failures = feature_vector_df[testdf > 2]
    multi_bit_failures = multi_bit_failures.reset_index(drop=True)
    if msocket:
        try:
            multi_socket_multibits = multi_bit_failures.groupby(['sid']).agg({'memoryid': list})
            multi_socket_multibits = multi_socket_multibits[multi_socket_multibits['memoryid'].swifter.apply(lambda x: len(set(x))) > 1]
            multi_socket_multibits = multi_socket_multibits.reset_index()
            multi_socket_multibits = multi_socket_multibits[['sid']]
            multi_socket_multibits = multi_bit_failures.join(multi_socket_multibits.set_index('sid'), on='sid', how='inner')
            multi_socket_multibits = multi_socket_multibits.reset_index(drop=True)
            multi_socket = find_multisocket(multi_socket_multibits)

            # join two tables to get index of multi_socket
            multi_socket = multi_bit_failures.join(multi_socket.set_index(['sid','memoryid']), on=['sid','memoryid'], how='inner', lsuffix='', rsuffix='_right')
            # drop right suffix columns
            multi_socket = multi_socket.drop([col for col in multi_socket.columns if col.endswith('_right')], axis=1)

        except:
            multi_socket = pd.DataFrame(columns=multi_bit_failures.columns)
        
        multi_bit_failures = multi_bit_failures[~multi_bit_failures.index.isin(multi_socket.index)]
    else:
        multi_socket = pd.DataFrame(columns=multi_bit_failures.columns)

    if mrank:
        single_rank = multi_bit_failures[multi_bit_failures['rankid'].swifter.apply(lambda x: len(set(x))) == 1]
    else:
        single_rank = multi_bit_failures
    single_rank = single_rank.reset_index(drop=True)
    single_column = single_rank[single_rank['col'].swifter.apply(lambda x: len(set(x)))==1 & (single_rank['bankid'].swifter.apply(lambda x: len(set(x)))==1)]
    single_row = single_rank[single_rank['row'].swifter.apply(lambda x: len(set(x)))==1 & (single_rank['bankid'].swifter.apply(lambda x: len(set(x)))==1)]
    single_bank = single_rank[(single_rank['col'].swifter.apply(lambda x: len(set(x)))>1 ) & (single_rank['row'].swifter.apply(lambda x: len(set(x)))>1 ) & (single_rank['bankid'].apply(lambda x: len(set(x)))==1)]
    multi_bank = single_rank[(single_rank['bankid'].swifter.apply(lambda x: len(set(x)))>1 )]

    # if feature_vector_df is dask dataframe, then compute it
    if feature_vector_df.__class__ == dask.dataframe.core.DataFrame:
        with dask.diagnostics.ProgressBar():
            multiple_single_bit_failures, single_rank, single_column, single_row, single_bank, multi_bank, multi_bit_failures \
            = dask.compute(multiple_single_bit_failures, single_rank, single_column, single_row, single_bank, multi_bank, multi_bit_failures)
    
    #remove single_column, row, bank, multibank from single_rank
    single_rank = single_rank[~single_rank.index.isin(single_column.index)]
    single_rank = single_rank[~single_rank.index.isin(single_row.index)]
    single_rank = single_rank[~single_rank.index.isin(single_bank.index)]
    single_rank = single_rank[~single_rank.index.isin(multi_bank.index)]
    
    if mrank:
        single_rank = single_rank[~single_rank.index.isin(multi_socket.index)]
        multi_rank = multi_bit_failures[multi_bit_failures['rankid'].swifter.apply(lambda x: len(set(x))) > 1]
        multi_rank = multi_rank.reset_index(drop=True)
    else:
        multi_rank = pd.DataFrame(columns=multi_bit_failures.columns)

    # make first result using above dataframes need deep copy
    # compute entire dask dataframe
    
    logical_result = {'single_rank': single_rank.copy(), 'multi_rank': multi_rank.copy(), 'multi_bank': multi_bank.copy(), 'multi_socket': multi_socket.copy(), 'single_bank': single_bank.copy(), 'single_row': single_row.copy(), 'single_column': single_column.copy(), 'multiple_single_bit_failures': multiple_single_bit_failures.copy()}

    # single row error
    local_wordline = single_row
    # single sense amplifier error means two clusters of error. 
    # first, remove local bitline error
    single_sense_amp = single_column.copy()
    # single_sense_amp, single_sbl_column, not_clustered_single_column
    single_sense_amp = single_sense_amp[single_sense_amp['row'].swifter.apply(lambda x: len(set(x))) <= 2**11]
    single_sense_amp = single_sense_amp[single_sense_amp['row'].swifter.apply(lambda x: np.array(x)[np.array(x) > min(x)+2**10]).swifter.apply(lambda x: max(x)-min(x) if len(x)>0 else 0) <= 2**10]
    single_sense_amp = single_sense_amp[single_sense_amp['row'].swifter.apply(lambda x: max(x)-min(x)) <= 2**16]
    not_clustered_single_column = single_column[ ~single_column.index.isin(single_sense_amp.index)]

    # single column caused by column decoder
    # is there any error after 64k from the min error?
    decoder_single_col = not_clustered_single_column[not_clustered_single_column['row'].swifter.apply(lambda x: np.array(x)-min(x)).swifter.apply(lambda x: max(x)-min(x)) >= 63*1024]
    not_clustered_single_column = not_clustered_single_column[ ~not_clustered_single_column.index.isin(decoder_single_col.index)]
    
    # single_csl_column <==> Remapping logics in csl (subbank 16K granularity failure based on histogram of max - min)
    single_csl_column = not_clustered_single_column[(not_clustered_single_column['row'].swifter.apply(lambda x: max(x)-min(x)) <= 2**14+1024 ) |\
         (((not_clustered_single_column.DRAM_model == ('A1'))|(not_clustered_single_column.DRAM_model == ('A2')))  \
            & (not_clustered_single_column['row'].swifter.apply(lambda x:max(x)-min(x)) <= 2**13+1024 ))]
    '''
    single_csl_column =\
            single_csl_column[single_csl_column['row'].apply(lambda x: np.array(x)[np.array(x) > min(x)+2**11])\
            .apply(lambda x: np.array(x)[np.array(x) > min(x)+2**11] if x.any() else np.array([0]))\
                .apply(lambda x: np.array(x)[np.array(x) > min(x)+2**11] if x.any() else np.array([0]))\
                    .apply(lambda x: np.array(x)[np.array(x) > min(x)+2**11] if x.any() else np.array([0])).apply(len) <= 2]
    '''
    not_clustered_single_column = not_clustered_single_column[~not_clustered_single_column.index.isin(single_csl_column.index)]
    try:
        local_wordline_two_clusters = single_bank[single_bank['row'].swifter.apply(lambda x: len(set(x)) == 2)]
        local_wordline_two_clusters = local_wordline_two_clusters[local_wordline_two_clusters['col'].swifter.apply(lambda x: len(set(x))) > 2]
        local_wordline_two_clusters = local_wordline_two_clusters[local_wordline_two_clusters['row'].swifter.apply(lambda x: (max(x)-min(x) >= 2**16) & (max(x)-min(x) <= 2**16))]

        single_bank = single_bank[~single_bank.index.isin(local_wordline_two_clusters.index)]
    except:
        local_wordline_two_clusters = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])

    # single bank, multi row
    # subarray row decoder, global row decoder, local word line select generator

    # consequtive_rows is part of single row error
    try:
        consequtive_rows = single_bank[(single_bank['row'].swifter.apply(lambda x: max(x)-min(x)) <= 4)]
        consequtive_rows = consequtive_rows[consequtive_rows['col'].swifter.apply(lambda x: len(set(x))) > 2]
        single_bank = single_bank[~single_bank.index.isin(consequtive_rows.index)]
    except:
        consequtive_rows = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])

    try:
        subarray_row_decoder = single_bank[(single_bank['row'].swifter.apply(lambda x: max(x)-min(x)) <= 2**10)] # 2**17 / 32 (number of subarray)
        subarray_row_decoder = subarray_row_decoder[subarray_row_decoder['col'].swifter.apply(lambda x: len(set(x))) > 2] # 2**17 / 32 (number of subarray)
        not_clustered_single_bank = single_bank[~single_bank.index.isin(subarray_row_decoder.index)]
    except:
        subarray_row_decoder = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])

    try:
        subarray_row_decoder_two_clusters = not_clustered_single_bank[not_clustered_single_bank['col'].swifter.apply(lambda x: len(set(x))) > 2]
        subarray_row_decoder_two_clusters = subarray_row_decoder_two_clusters[\
            subarray_row_decoder_two_clusters['row'].swifter.apply(lambda x: np.array(x)[min(np.array(x)[np.array(x) >= min(x)+2**10])-x>= 62*1024]).apply(len) > 0]
        not_clustered_single_bank = single_bank[~single_bank.index.isin(subarray_row_decoder.index) & ~single_bank.index.isin(subarray_row_decoder_two_clusters.index)]
    except:
        subarray_row_decoder_two_clusters = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])

    try:
        if not_clustered_single_bank.empty:
            lwl_sel = not_clustered_single_bank
        else:
            lwl_sel = not_clustered_single_bank[not_clustered_single_bank['row'].swifter.apply(lambda x: len(set(x))>2)]
        lwl_sel = lwl_sel[lwl_sel.col.swifter.apply(lambda x: len(set(x))) > 2]
        # range from 0 to 64 and 1023 to 1023-64
        # 64 come from the fact that MWL is 64. when FX is wrong, it will be repeated, and 64 is the maximum for error region
        rng = [ i for i in range(0,64)] + [i for i in range(1024-1,1024-1-64,-1)]
        lwl_sel = lwl_sel[lwl_sel.row.swifter.apply(lambda x: sorted(list(set(x)))).swifter.apply(lambda x: set(np.diff(x)%(2**10))).swifter.apply(lambda x: all([i in [j for j in rng] for i in x]))]

        # Needs to span for 64k rows
        lwl_sel = lwl_sel[lwl_sel.row.swifter.apply(lambda x: max(x)-min(x) >= 2**16)]
        not_clustered_single_bank = not_clustered_single_bank[~not_clustered_single_bank.index.isin(lwl_sel.index)]
    except:
        lwl_sel = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])
    # subbank and subarray level repeat
    try:
        lwl_sel2 = not_clustered_single_bank[not_clustered_single_bank['row'].swifter.apply(lambda x: len(set(x))>2)]
        lwl_sel2 = lwl_sel2[lwl_sel2.col.swifter.apply(lambda x: len(set(x))) > 2]
        # range from 0 to 1024 and 16k to 16k-1024
        rng = [ i for i in range(0,1024)] + [i for i in range(2**14-1,2**14-1-1024,-1)]
        lwl_sel2 = lwl_sel2[lwl_sel2.row.swifter.apply(lambda x: sorted(list(set(x)))).swifter.apply(lambda x: set(np.diff(x)%(2**14))).swifter.apply(lambda x: all([i in [j for j in rng] for i in x]))]
        
        # Needs to span for 64k rows
        lwl_sel2 = lwl_sel2[lwl_sel2.row.swifter.apply(lambda x: max(x)-min(x) >= 2**16)]
        not_clustered_single_bank = not_clustered_single_bank[~not_clustered_single_bank.index.isin(lwl_sel2.index)]
    except:
        lwl_sel2 = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])

    try:
        global_row_decoder_two_clusters = not_clustered_single_bank[not_clustered_single_bank['col'].swifter.apply(lambda x: len(set(x))) > 2]
        global_row_decoder_two_clusters = global_row_decoder_two_clusters[\
            global_row_decoder_two_clusters['row'].swifter.apply(lambda x:min(np.array(x)[np.array(x) >= min(x)+16*1024])-x).swifter.apply(lambda x: np.array(x)[x>0]).swifter.apply(lambda x: x>=62*1024).swifter.apply(all)]
        not_clustered_single_bank = not_clustered_single_bank[ ~not_clustered_single_bank.index.isin(global_row_decoder_two_clusters.index)]
    except:
        global_row_decoder_two_clusters = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])

    # single bank, multi column
    try:
        decoder_multi_col = not_clustered_single_bank
        temp = decoder_multi_col['row'].swifter.apply(min)
        decoder_multi_col = decoder_multi_col[decoder_multi_col['row'].swifter.apply(lambda x: np.array(x)[np.array(x) >= min(x)+2**14]).swifter.apply(lambda x: min(x) if len(x)>0 else 0)-temp >= 63*1024]
        decoder_multi_col = decoder_multi_col[decoder_multi_col['col'].swifter.apply(lambda x: len(set(x))) == 2]
        not_clustered_single_bank = not_clustered_single_bank[~not_clustered_single_bank.index.isin(decoder_multi_col.index)]
    except:
        decoder_multi_col = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])
    
    try:
        single_csl_bank = not_clustered_single_bank[(not_clustered_single_bank['row'].apply(lambda x: max(x)-min(x)) <= 2**14+1024 ) |\
         (((not_clustered_single_bank.DRAM_model == ('A1'))|(not_clustered_single_bank.DRAM_model == ('A2')))  \
            & (not_clustered_single_bank['row'].apply(lambda x:max(x)-min(x)) <= 2**13+1024 ))]

        single_csl_bank = single_csl_bank[single_csl_bank['col'].apply(lambda x: len(set(x)))==2]
        not_clustered_single_bank = not_clustered_single_bank[~not_clustered_single_bank.index.isin(single_csl_bank.index)]
    except:
        single_csl_bank = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])
    
    try:
        mutli_csls = not_clustered_single_bank[(not_clustered_single_bank['row'].apply(lambda x: max(x)-min(x)) <= 2**14+1024 ) |\
         (((not_clustered_single_bank.DRAM_model == ('A1'))|(not_clustered_single_bank.DRAM_model == ('A2')))  \
            & (not_clustered_single_bank['row'].apply(lambda x:max(x)-min(x)) <= 2**13+1024 ))]
        not_clustered_single_bank = not_clustered_single_bank[~not_clustered_single_bank.index.isin(mutli_csls.index)]
    except:
        mutli_csls = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])



    #multi bank 
    # bank control, row_addr_mux

    # multi bank, but if it only affects at most 2K rows, then it is actually single_sense_amp
    try:
        bank_control = multi_bank[multi_bank['col'].apply(lambda x:len(set(x))) <= 2]
        bank_control = bank_control[bank_control.row.apply(len)>=4]
        not_clustered_multi_bank = multi_bank[~multi_bank.index.isin(bank_control.index)]
    except:
        bank_control = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])
    try:
        potential_sense_amp = bank_control[bank_control.error_type != "read"]
        potential_positions = potential_sense_amp.groupby(['sid']).agg({'row':lambda x: sum(x.apply(len))})
        potential_positions.rename(columns={'row':'row_num'}, inplace=True)
        potential_sense_amp = potential_sense_amp.reset_index()
        potential_sense_amp = potential_sense_amp.merge(potential_positions, on='sid', how='left')
        potential_sense_amp = potential_sense_amp.set_index('index')
        potential_csl_column = potential_sense_amp[potential_sense_amp.row_num > 2048]
        potential_sense_amp = potential_sense_amp[potential_sense_amp.row_num <=2048]
        # potential_sense_amp should not have duplicated sid if there is duplicated sid, then drop both
        # if it is multi module error, sum the counter of errors up and compare with 2048
        # potential_sense_amp = potential_sense_amp.groupby(['sid']).agg({'row':lambda x: sum(x.apply(len))}).reset_index()
        # potential_sense_amp = potential_sense_amp[~potential_sense_amp.sid.duplicated(keep=False)]


        
        bank_control = bank_control[~bank_control.index.isin(potential_sense_amp.index)]
        bank_control = bank_control[~bank_control.index.isin(potential_csl_column.index)]

    except:
        potential_sense_amp = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])
        potential_csl_column = pd.DataFrame(columns=['sid','memoryid','rankid','bankid','row','col','error_time_min','error_time_max','DRAM_model'])

    physiclal_result =  { 'multiple_single_bit_failures':multiple_single_bit_failures,
    'local_wordline':local_wordline, 'single_sense_amp':single_sense_amp, 'decoder_single_col':decoder_single_col, 'single_csl_column':single_csl_column, 'not_clustered_single_column':not_clustered_single_column,
    'single_csl_bank':single_csl_bank, 'subarray_row_decoder':subarray_row_decoder,
    'decoder_multi_col':decoder_multi_col, 'multi_socket': multi_socket, 'multi_rank':multi_rank,
    'global_row_decoder_two_clusters':global_row_decoder_two_clusters, 'mutli_csls':mutli_csls, 'lwl_sel2':lwl_sel2, 'lwl_sel':lwl_sel, 'bank_control':bank_control, 'not_clustered_single_bank':not_clustered_single_bank, 'not_clustered_multi_bank':not_clustered_multi_bank,
    "subarray_row_decoder_two_clusters":subarray_row_decoder_two_clusters, 'local_wordline_two_clusters':local_wordline_two_clusters,
    "consequtive_rows" : consequtive_rows,"potential_sense_amp":potential_sense_amp,"potential_csl_column":potential_csl_column}
    #retun all results
    return logical_result,physiclal_result

def conv_mapping(df,column_name):
    '''
    Coarse grained convergence using this mapping
    map values to camparison
    To : From
    SWD : local_wordline, local_wordline_two_clusters, subarray_row_decoder, subarray_row_decoder_two_clusters
    BLSA: single_sense_amp
    Col_decoder: decoder_single_col, decoder multi_col
    CSL: single_csl_column, single_csl_bank, mutli_csls
    Row_decoder: global_row_decoder_two_clusters, lwl_sel, lwl_sel2 
    Bank_patterns: bank_control, row_addr_mux
    multi_bank: not_clustered_multi_bank
    '''
    mapping = {'SWD': ['local_wordline', 'local_wordline_two_clusters', 'subarray_row_decoder', 'subarray_row_decoder_two_clusters'],
                'BLSA': ['single_sense_amp'],
                'Col_decoder': ['decoder_single_col', 'decoder_multi_col'],
                'CSL': ['single_csl_column', 'single_csl_bank', 'mutli_csls'],
                'Row_decoder': [ 'global_row_decoder_two_clusters', 'lwl_sel', 'lwl_sel2'],
                'Bank_patterns': ['bank_control', 'row_addr_mux'],
                'multi_bank': ['not_clustered_multi_bank']}
    df = df.copy()    
    for key in mapping.keys():
        df.loc[df[column_name].isin(mapping[key]),column_name] = key
    return df

def conv_mapping_onlykey(key_name):
    mapping = {'SWD': ['local_wordline', 'local_wordline_two_clusters', 'subarray_row_decoder', 'subarray_row_decoder_two_clusters'],
                'BLSA': ['single_sense_amp'],
                'Col_decoder': ['decoder_single_col', 'decoder_multi_col'],
                'CSL': ['single_csl_column', 'single_csl_bank', 'mutli_csls'],
                'Row_decoder': ['global_row_decoder_two_clusters', 'lwl_sel', 'lwl_sel2'],
                'Bank_patterns': ['bank_control', 'row_addr_mux'],
                'multi_bank': ['not_clustered_multi_bank']}
    for key in mapping.keys():
        if key_name in mapping[key]:
            return key
    return key_name

def groupby_machine_informations(df):
    # first row is the name of the column
    machines=pd.read_csv("inventory.csv",header=0)

    #using sid, join the machines dataframe with the df dataframe
    df = df.merge(machines[['sid','DRAM_model','DIMM_number','server_manufacturer']], on='sid')   
    return df

#plot horizontal lines on the plot on 8k 16k 24k 32k 40k 48k ... 128k
def plot_hlines(ax):
    for i in range(17):
        ax.axhline(y=i*8*1024, color='k', linestyle='--',alpha=0.3)
    return ax

'''
Coarse grained convergence using this mapping
map values to camparison
To : From
SWD : local_wordline, local_wordline_two_clusters, subarray_row_decoder, subarray_row_decoder_two_clusters
BLSA: single_sense_amp
Col_decoder: decoder_single_col, decoder multi_col
CSL: single_csl_column, single_csl_bank, mutli_csls
Row_decoder: global_row_decoder_two_clusters, lwl_sel, lwl_sel2 
Bank_patterns: bank_control, row_addr_mux
multi_bank: not_clustered_multi_bank
'''
def merge_dictionary(dic):
    mapping = {'SWD': ['local_wordline', 'local_wordline_two_clusters', 'subarray_row_decoder', 'subarray_row_decoder_two_clusters'],
                'BLSA': ['single_sense_amp'],
                'Col_decoder': ['decoder_single_col', 'decoder_multi_col'],
                'CSL': ['single_csl_column', 'single_csl_bank', 'mutli_csls'],
                'Row_decoder': ['global_row_decoder_two_clusters', 'lwl_sel', 'lwl_sel2'],
                'Bank_patterns': ['bank_control', 'row_addr_mux'],
                'multi_bank': ['not_clustered_multi_bank']}
    
    for key in mapping.keys():
        if key not in dic.keys():
            dic[key] = pd.DataFrame()
        for value in mapping[key]:
            if value in dic.keys():
                dic[key] = pd.concat([dic[key],dic[value]])
                del dic[value]
    return dic
