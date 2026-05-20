echo "ArrivalRate,device_write,cpus,memory,cpuLoad,Throughput,ResponseTime" > loadtest.csv
for device_write in "10mb"
do
    for cpu in 1
    do
        for mem in '500m'
        do
            for arr in {1..5..1};
            do
                for size in {100..200..50};
                do
                    for cpuLoad in 10
                    do
                        curl -X GET "http://10.51.0.100:8081/dockerStart.php?memory=${mem}&cpus=${cpu}&device-write=${device_write}"
                        httperf --client=0/1 --server=10.51.0.100 --port=81 --uri="/server.php?size=4&cpuLoad=${cpuLoad}" --send-buffer=4096 --recv-buffer=16384 --num-conns=50 --rate=${arr} > result.txt
                        cat result.txt >> result_all.txt
                        echo >> resultall.txt
                        echo "======================================================================" >> resultall.txt
                        echo >> resultall.txt
                        awk -v var0="$arr" -v var1="$device_write" -v var2="$cpu" -v var3="$mem" -v var4="$cpuLoad" '{OFS=","; if ($1 == "Reply" && $2 == "rate") t=$7; if ($1 == "Reply" && $2 == "time") r1 = $5;} END{print var0,var1,var2,var3,var4,t,r1}' result.txt >> loadtest.csv
                        curl -X GET "http://10.51.0.100:8081/dockerStop.php"
                    done
                done
            done
        done
    done
done