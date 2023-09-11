import os
import pandas as pd
from pathlib import Path
import matplotlib.pyplot as plt
from scipy.stats import chi2, loglaplace
from labellines import labelLine, labelLines
import numpy as np

#os.system('bash testing.sh')
NYEAR=5
_columns=['name','1-year','2-year','3-year','4-year', '5-year']

DUE = pd.DataFrame(columns=_columns)
CE = pd.DataFrame(columns=_columns)
SDC = pd.DataFrame(columns=_columns)
seeds = []
#UNIQ_SEED = "12120"
for UNIQ_SEED in ["0"]:
  for p in Path('./build/').glob(r'[0|4|3|2|5|6][3|0|1][4|3|2|1|0]*ModuleALL*'):
    
    name = p.name
    seeds.append(name.split('_')[0])
    print(name)
    txt = ''
    with open('./build/'+name,'r') as f:
      counter = 0
      lines = f.readlines()
      last_lines = lines[-20:]
      if len(last_lines) ==0:
        continue
      txt = last_lines[0]
      counter = 0
      while 'runs' not in txt:
        counter=counter+1
        txt = last_lines[counter]
        if counter > 20:
          break
      print(txt)
      counter += 1
      name_arr = name.split('.')
      try:
        position = name_arr.index(UNIQ_SEED)
      except:
        continue
      name = '.'.join(name.split('.')[:position]) + '.'.join(name.split('.')[-2:])
      
      tmp = [name]
      for i in range(NYEAR):
        counter = counter+1
        tmp.append(last_lines[counter])
      CE.loc[CE.shape[0]] = tmp

      counter+=1 
      tmp = [name]
      for i in range(NYEAR):
        counter = counter+1
        tmp.append(last_lines[counter])
      DUE.loc[DUE.shape[0]] = tmp
      
      counter+=1 
      tmp = [name]
      for i in range(NYEAR):
        counter = counter+1
        tmp.append(last_lines[counter])
      SDC.loc[SDC.shape[0]] = tmp

CE = CE.apply(pd.to_numeric,errors='ignore')
DUE =DUE.apply(pd.to_numeric,errors='ignore')
SDC =SDC.apply(pd.to_numeric,errors='ignore')

CE = CE.set_index('name').transpose()
DUE = DUE.set_index('name').transpose()
SDC = SDC.set_index('name').transpose()

CE = CE.reindex(sorted(CE.columns), axis=1)
DUE = DUE.reindex(sorted(DUE.columns), axis=1)
SDC = SDC.reindex(sorted(SDC.columns), axis=1)


print(CE)
print(DUE)
print(SDC)
CE.to_csv('CE.csv')
DUE.to_csv('DUE.csv')
SDC.to_csv('SDC.csv')

'''
CE.plot(title='CE',logy=True,ylim=[1e-9,1],legend=False)
labelLines(plt.gca().get_lines(), align=False, fontsize=8)
DUE.plot(title='DUE',logy=True,ylim=[1e-9,1],legend=False)
labelLines(plt.gca().get_lines(), align=False, fontsize=8)
SDC.plot(logy=True,title='SDC',ylim=[1e-9,1],legend=False)
labelLines(plt.gca().get_lines(), align=False, fontsize=8)
plt.show()
plt.close()

# split per each seed, show the simple and new model

for seed in seeds:
  tCE = CE.filter(regex='^'+seed)
  tDUE = DUE.filter(regex='^'+seed)
  tSDC = SDC.filter(regex='^'+seed)
  tCE.plot(title='CE',logy=True,ylim=[1e-9,1],legend=False)
  labelLines(plt.gca().get_lines(), align=False, fontsize=8)
  plt.savefig('figure/CE_'+seed+'.png')
  plt.close()
  tDUE.plot(title='DUE',logy=True,ylim=[1e-9,1],legend=False)
  labelLines(plt.gca().get_lines(), align=False, fontsize=8)
  plt.savefig('figure/DUE_'+seed+'.png')
  plt.close()
  tSDC.plot(logy=True,title='SDC',ylim=[1e-9,1],legend=False)
  labelLines(plt.gca().get_lines(), align=False, fontsize=8)
  plt.savefig('figure/SDC_'+seed+'.png')
  plt.close()

pass
'''
