#rm *.i6.1e-1.1e-5.1e-7

idx=12
while true; do
    count=$(pgrep -c faulterrorsim)
    if [ $count -lt 16 ]; then
        echo "Found $count instances of faulterrorsim. Starting execution."
        break
    else
        echo "Found $count instances of faulterrorsim. Waiting for 1 minute before checking again."
        sleep 60
    fi
done


for module in 4 
do
for i in 5011 5012 #5001 5002 4019 4020 4021 4022 4031 4032
do
# ./faulterrorsim ${i} 1000000 ${idx} S i9 0 1e-5 1e-5 ${module} > results_${module}/${i}_${idx}_output_1e-5_1e-5.txt &
# ./faulterrorsim ${i} 1000000 ${idx} S i9 0 1e-6 1e-5 ${module} > results_${module}/${i}_${idx}_output_1e-6_1e-5.txt & 
#./faulterrorsim ${i} 1000000 ${idx} S i9 0 1e-7 1e-5 ${module} > results_${module}/${i}_${idx}_output_1e-7_1e-5.txt &  
#./faulterrorsim ${i} 1000000 ${idx} S i9 0 1e-8 1e-5 ${module} > results_${module}/${i}_${idx}_output_1e-8_1e-5.txt &
./faulterrorsim ${i} 494096  ${idx} S i9 0 0 1e-5 ${module} > results_${module}/${i}_${idx}_output_0_0.txt &
done
done
wait


