#include <random>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <semaphore.h>

using namespace std;
const char selection_area[52] ={'A','a','B', 'b','C', 'c','D', 'd','E', 'e', 'F', 'f', 'G' ,'g','H', 'h','I' ,'i','J', 'j','K', 'k','L', 'l','M' ,'m','N', 'n','O' ,'o','P' ,'p','Q' ,'q','R' ,'r','S' ,'s','T', 't','U' ,'u','V', 'v','W' ,'w','X' ,'x','Y' ,'y', 'Z' ,'z'};
int interval_A = 10; // time interval for crawler (microsecond)
int interval_B = 10; // time interval for classifier (microsecond)
int key = 1;
int textCount = 0;
int class_label;
int crawl_cond;
const int NUM_CRAWLER = 3;
const int bufferLength = 12;
const int tableWidth = 15;
static const int ALEN=50;
int threadid[NUM_CRAWLER];
int print_enough_message = 1;
bool quit_crawler = false;
pthread_t threads[NUM_CRAWLER], threadv;
int i, x, y;
int classlabel_p[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
string result[1000];
pthread_mutex_t  crawler_lock = PTHREAD_MUTEX_INITIALIZER; // to lock the crawler when the queue is full
pthread_mutex_t  classifier_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print = PTHREAD_MUTEX_INITIALIZER;
sem_t classify; // semaphore for classifier to limit the access of classifier when queue is empty
sem_t crawl; // semaphore for crawler to limit the access of crawler when queue is full

void printlist(){ // function to output the list in table format
    pthread_mutex_lock(&print);
    cout  << right << "crawler1" << setw(tableWidth) << right << "crawler2" << setw(tableWidth) << right << "crawler3" << setw(tableWidth) << right << "classifier" << endl;
    pthread_mutex_unlock(&print);
}
void printcrawler1(string t){ // function to output the result of crawler 1 in table format
    pthread_mutex_lock(&print);
    cout << right << t << setw(tableWidth) << right << "        " << setw(tableWidth) << right << "        " << setw(tableWidth) << right <<  "          "<< endl;;
    pthread_mutex_unlock(&print);
}
void printcrawler2(string t){ // function to output the result crawler 2 in table format
    pthread_mutex_lock(&print);
    cout << right << "        " << setw(tableWidth) << right << t << setw(tableWidth) << right << "        " << setw(tableWidth) << right <<  "          "<< endl;;
    pthread_mutex_unlock(&print);
}
void printcrawler3(string t){ // function to output the result crawler 3 in table format
    pthread_mutex_lock(&print);
    cout << right << "        " << setw(tableWidth) << right << "        " << setw(tableWidth) << right << t << setw(tableWidth) << right <<  "          " << endl;
    pthread_mutex_unlock(&print);
}
void printclassifier(string t){ // function to output the result classifer in table format
    pthread_mutex_lock(&print);
    cout << right << "        " << setw(tableWidth) << right << "        " << setw(tableWidth) << right << "        " << setw(tableWidth) << right << t << endl;
    pthread_mutex_unlock(&print);
}

void printStatus(string input)
{
    if (pthread_self() == threads[0]){
        printcrawler1(input);
    }
    else if (pthread_self() == threads[1]){
        printcrawler2(input);
    }
    else if (pthread_self() == threads[2]){
        printcrawler3(input);
    }
    else{
        printclassifier(input);
    }
}

class Queue { //queue list of string for buffer 
    private:
        string buffer[bufferLength];
        int count;
    public:
        Queue() {
            count = 0;
        }
        int getQueueLength() { return count;}
        bool queueFull() { return (count == bufferLength); }
        void enqueue(string grabbedText) {
            if(!queueFull()){
                buffer[count] = grabbedText;
                count++;
                sem_post(&classify);
            }
        }
        string getQueue(){
            return buffer[0];
        }
        void updateBuffer(){
            if(count>1){
                for (i = 0; i < count - 2; i++){
                    buffer[i]=buffer[i+1];
                }
                int delete_location = count - 1;
                buffer[delete_location].clear();
                count--;   
                sem_post(&crawl);
            }
            else{
                int delete_location = count - 1;
                buffer[delete_location].clear();
                count--;
                sem_post(&crawl);
            }          
            
        }
};

int k_enough(){ // check if every class label have 5 or more element 
    for (i = 1; i <= 13; i++){
        if (classlabel_p[i] < 5){
            return 0;
        }
    }
    return 1;
}

int getLabel(string grabbed){
    for (int i = 0; i<grabbed.length(); i++){
        for (int x = 0; x<52; x++){
            if(grabbed.at(i) == selection_area[x])
                return tolower(grabbed.at(i)
                );
        }
    }
    return EXIT_FAILURE;
}

char* str_generator(void){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(48, 122);
    char *art = new char[ALEN+3];

    for (int i=0; i<ALEN; ++i)
        art[i] = char(dis(gen));
    art[ALEN] = 0;
    return art;
}

void* crawler(void *arg){
    bool wait = false;
    Queue *queue = *(Queue**)arg;
    while (!quit_crawler) {   
        //grab section
        sem_getvalue(&crawl, &crawl_cond);
        if(wait == true && !queue->queueFull() && crawl_cond > 0){
            wait == false;
            printStatus("s-wait");
            printStatus(to_string(queue->getQueueLength()));           
        }
        else if(wait == true){
            continue;
        }
        printStatus("grab");
        string text(str_generator()); //  constructor for convert char* to string
        //f-grab section
        printStatus("f-grab");
        printStatus(to_string(queue->getQueueLength()));
        sem_getvalue(&crawl, &crawl_cond);
        if(queue->queueFull()){
            wait = true;
            printStatus("wait");           
            continue;   
        }
        sem_wait(&crawl);
        pthread_mutex_lock(&crawler_lock);
        queue->enqueue(text);
        wait = false;      
        usleep(interval_A);
        pthread_mutex_unlock(&crawler_lock);
    }  
    printStatus("quit");
    pthread_cancel(pthread_self());
}


void *classifier(void* arg){
    Queue *queue = *(Queue**)arg;
	while ((!k_enough()) || (queue->getQueueLength() != 0)) {
        sem_wait(&classify);
        printStatus("clfy");
        int new_label = getLabel(queue->getQueue());
        class_label = int(new_label-'a')%13+1;
        string input;
        string string_key = to_string(key);
        string string_class_label = to_string(class_label);
        input= string_key + " " + string_class_label + " " + queue->getQueue();
        result[textCount] = input;
        classlabel_p[class_label] += 1;
        key++;
        textCount++;
        pthread_mutex_lock(&classifier_lock);
        queue->updateBuffer();
        pthread_mutex_unlock(&classifier_lock);
        printStatus("f-clfy");
        //signal crawler to stop wait
        usleep(interval_B);
        if(k_enough() && print_enough_message != 0){
            string no_of_text = to_string(textCount);
            string enough_message = no_of_text + "-enough";
            printclassifier(enough_message);
            print_enough_message --;
            quit_crawler = true;
        }
	}
    string no_of_text = to_string(textCount);
    string store_message = no_of_text + "-store";
    printStatus(store_message);
    printStatus("quit");
    pthread_cancel(pthread_self());
}

int main(){
    int i, rc;
    void *status;
    Queue *queue = new Queue();
    sem_init(&classify, 0, 0);
    sem_init(&crawl,0 , 12);
    printlist(); // print the table list
    for (i = 0; i < NUM_CRAWLER; i++) {
        threadid[i] = i;
        rc = pthread_create(&threads[i], NULL, crawler, (void *)&queue);       
        if (rc) {
            cout << "Error when creating crawler!" << endl;
            exit(-1);
        }
        if (i == 0){
            printcrawler1("start");
        }
        else if (i == 1){
            printcrawler2("start");
        }
        else if (i == 2){
            printcrawler3("start");
        }
    }
    rc = pthread_create(&threadv, NULL, classifier, (void *)&queue);
    if (rc) {
        cout << "Error when creating classifier!" << endl;
        exit(-1);
    }
    printclassifier("start");
    for (i = 0; i < NUM_CRAWLER; i++) {
		rc = pthread_join(threads[i], &status);
		if (rc) {
			cout << "Error when joining crawler!" << endl;
			exit(-1);
		}
	}
    rc = pthread_join(threadv, NULL);
    if (rc) {
			cout << "Error when joining classifier!" << endl;
			exit(-1);
	}
    sem_destroy(&classify);
    sem_destroy(&crawl);
    for (i = 0; i < textCount; i++){
        cout << endl << result[i] << endl;
    }
	pthread_exit(NULL);
    return 1;
}