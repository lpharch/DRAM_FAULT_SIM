import os
import pandas as pd
from pathlib import Path
import matplotlib.pyplot as plt
from scipy.stats import chi2, loglaplace
from labellines import labelLine, labelLines

#os.system('bash testing.sh')
_columns=['name','SBIT','SWORD','SCOL','SROW','SBANK','MBANK','MRANK','CHANNEL','INHERENT1','INHERENT2','INHERENT3','INHERENT4','INHERENT5','INHERENT6']

DUE = pd.DataFrame(columns=_columns)
ALL = pd.DataFrame(columns=_columns)
SDC = pd.DataFrame(columns=_columns)

working_dir = "results/"
NERRORS=14
for UNIQ_SEED in ["1235","12341234"]:
  for p in Path('.').glob(working_dir+r'/[4|3|2][3|0|1|2]*_'+UNIQ_SEED+'*'+'1e-7'+'*'):
    name = p.name
    print(name)
    txt = ''
    with open(working_dir+name,'r') as f:
      counter = 0
      lines = f.readlines()
      last_lines = lines[-60:]
      if len(last_lines) ==0:
        continue
      txt = last_lines[0]
      counter = 0
      flag = True
      while 'SDC' not in txt:
        counter=counter+1
        txt = last_lines[counter]
        if counter > 50:
          flag=False
          break
      if flag:
          print(txt)
          name_arr = name.split('_')
          try:
            position = name_arr.index(UNIQ_SEED)
          except:
            continue
          name = '_'.join(name.split('_')[:position+1]) 
          tmp = [name]
          for i in range(NERRORS):
            counter = counter+1
            tmp.append(last_lines[counter].split()[1])
          SDC.loc[SDC.shape[0]] = tmp
      
      counter = 0
      while 'DUE' not in txt:
        counter=counter+1
        txt = last_lines[counter]
        if counter > 50:
          break
      tmp = [name]
      for i in range(NERRORS):
        counter = counter+1
        tmp.append(last_lines[counter].split()[1])
      DUE.loc[DUE.shape[0]] = tmp

      counter = 0
      while 'entire' not in txt:
        counter=counter+1
        txt = last_lines[counter]
        if counter > 50:
          break
      tmp = [name]
      for i in range(NERRORS):
        counter = counter+1
        tmp.append(last_lines[counter].split()[1])
      ALL.loc[ALL.shape[0]] = tmp


ALL =ALL.apply(pd.to_numeric,errors='ignore')
DUE =DUE.apply(pd.to_numeric,errors='ignore')
SDC =SDC.apply(pd.to_numeric,errors='ignore')

ALL = ALL.set_index('name').transpose()
DUE = DUE.set_index('name').transpose()
SDC = SDC.set_index('name').transpose()

ALL = ALL.reindex(sorted(DUE.columns), axis=1)
DUE = DUE.reindex(sorted(DUE.columns), axis=1)
SDC = SDC.reindex(sorted(SDC.columns), axis=1)

print(ALL)
print(DUE)
print(SDC)
ALL.to_csv('ALL.csv')
DUE.to_csv('DUE.csv')
SDC.to_csv('SDC.csv')

ALL.plot(title='CE',logy=True,ylim=[1e-9,1],legend=False)
labelLines(plt.gca().get_lines(), align=False, fontsize=8)
DUE.plot(title='DUE',logy=True,ylim=[1e-9,1],legend=False)
labelLines(plt.gca().get_lines(), align=False, fontsize=8)
SDC.plot(logy=True,title='SDC',ylim=[1e-9,1],legend=False)
labelLines(plt.gca().get_lines(), align=False, fontsize=8)
plt.show()
