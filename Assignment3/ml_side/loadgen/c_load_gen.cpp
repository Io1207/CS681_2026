
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

/**
 * ARGUMENTS
 * arg1 - Number of Users
 * arg2 - Think Time (seconds)
 * arg3 - Test Duration (seconds)
 * arg4 - Server URL
 * arg5 - Log files prefix
 */


int NUM_USERS;
int THINK_TIME;
int TEST_DUR;
string URL;
string FILE_PREF;

long long epoch_milis(auto tp)
{
    return chrono::duration_cast<chrono::milliseconds>(
        tp.time_since_epoch()
    ).count();
}

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
  /* we are not interested in the downloaded bytes itself,
     so we only return the size we would have saved ... */
  *(int*)data = (size * nmemb);
  return (size_t)(size * nmemb);
}

int user(int uid)
{
    map<long, int> http_ret_count;

    ofstream logfile;
    string logfilename = ((FILE_PREF +  "userlog") + to_string(uid) )+ ".log";
    logfile.open(logfilename);

    CURL *curl;
    CURLcode result;

    result = curl_global_init(CURL_GLOBAL_ALL);
    if(result)
    {
        cout << "here\n";
        return (int)result;
    }

    curl = curl_easy_init();

    int num_bytes = 0;

    if(curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &num_bytes);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)TEST_DUR);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)TEST_DUR);
    }
    else
    {
        return 1;
    }

    // handle is set up;
    using clock = chrono::system_clock;
    auto start_of_test = clock::now();

    while(clock::now() < start_of_test + chrono::seconds(TEST_DUR))
    {
        //record sending time
        auto snd_time = clock::now();

        //send - blocking
        CURLcode ret = curl_easy_perform(curl);

        // record rcv time 
        auto rcv_time = clock::now();

        if(ret)
        {
            logfile << uid << " " << "curl error " << curl_easy_strerror(ret) << "\n";

            // std::cerr << "URL: " << URL << "\n";
        }
        else
        {
            long http_ret;
            curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE, &http_ret);

            // log snd, rcv, return_code
            logfile << uid << " " << "snd " << epoch_milis(snd_time) << "\n";
            logfile << uid << " " << "rcv " << epoch_milis(rcv_time) << " " << http_ret << "\n";
            // http_ret_count[http_ret]++;
            logfile.flush();
        }


        while(clock::now() < rcv_time + chrono::seconds(THINK_TIME) && (ret == 0))
        {
            this_thread::sleep_until(rcv_time + chrono::seconds(THINK_TIME));
        }
    }


    curl_easy_cleanup(curl);
    return 0;
}

int main(int argc, char * argv[])
{
    if(argc < 6)
        exit(7);
    NUM_USERS  = atoi(argv[1]);
    THINK_TIME = atoi(argv[2]);
    TEST_DUR   = atoi(argv[3]);
    URL = argv[4];
    FILE_PREF = argv[5];

    vector<int> pids;
    for(int i=0;i<NUM_USERS;i++)
    {
        int ret = fork();
        if(ret < 0)
        {
            cerr << "Fork " << i << " failed, retrying.";
            perror("Error in fork");
            i--;
        }
        else if(ret == 0)
        {
            int pret = user(i);
            exit(pret);
        }
        else
        {
            pids.push_back(ret);
        }
    }

    map<int, int> ret_codes;
    for(auto pid : pids)
    {
        int status;
        waitpid(pid , &status, 0);
        ret_codes[status]++;
    }

    std::cout << "Return code \tcount\n";
    for(auto &[code,cnt] : ret_codes)
        cout << code << "\t\t" << cnt << "\t" << curl_easy_strerror((CURLcode)code) << "\n";
}