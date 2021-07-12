// EID: chlein2 SID:55235844 Name:Lein Chun Hang
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
using namespace std;

#define MAX_LOOP 1000
#define time_quantum 5
bool mutex_lock = false; // store the mutex status


void lock(){
    mutex_lock = true;
}

void unlock(){
    mutex_lock = false;
}

struct service_t{
	string type;	// C (cpu), D (disk), K (keyboard), L (lock), U (unlock)
	int time_cost;
	service_t(): type(""), time_cost(-1) {}
	service_t(string type, string desc): type(type) {
        if (desc.compare("mtx") == 0){  // set time cost to 0 if the service type is lock or unlock
            this->time_cost = 0;
        }
        else{
            this->time_cost = stoi(desc);
        }
	}
};

struct process_t
{
	int process_id;
	int fb_level;
	int arrival_time;
	vector<service_t> service_seq;
	service_t cur_service;
	int cur_service_idx;
	int cur_service_tick;	// num of ticks that has been spent on current service
	vector<int> working;	// working sequence on CPU, for loging output

	// Call when current service completed
	// if there are no service left, return true. Otherwise, return false
	bool proceed_to_next_service()
	{
		this->cur_service_idx++;
		this->cur_service_tick = 0;
		if(this->cur_service_idx >= this->service_seq.size()) {	// all services are done, process should end
			return true;
		}
		else {		// still requests services
			this->cur_service = this->service_seq[this->cur_service_idx];
			return false;
		}
	};

	bool check_end(){
		if(this->cur_service_idx >= this->service_seq.size()) {	// all services are done, process should end
			return true;
		}
		else {		// still requests services
			return false;
		}
	}
	// Log the working ticks on CPU (from `start_tick` to `end_tick`)
	void log_working(int start_tick, int end_tick)
	{
		this->working.push_back(start_tick);
		this->working.push_back(end_tick);
	};

	void lower_fb(){
		if(this->fb_level < 2){
			this->fb_level++;
		}	
	}

	int get_fblevel(){
		return this->fb_level;
	}
};

// write output log
int write_file(vector<process_t> processes, const char* file_path)
{
    ofstream outputfile;
    outputfile.open(file_path);
    for (vector<process_t>::iterator p_iter = processes.begin(); p_iter != processes.end(); p_iter++) {
    	outputfile << "process " << p_iter->process_id << endl; 
      	for (vector<int>::iterator w_iter = p_iter->working.begin(); w_iter != p_iter->working.end(); w_iter++) {
        	outputfile << *w_iter << " ";
      	}
      	outputfile << endl;
    }
    outputfile.close();
    return 0;
}

// Split a string according to a delimiter
void split(const string& s, vector<string>& tokens, const string& delim=" ") 
{
	string::size_type last_pos = s.find_first_not_of(delim, 0); 
	string::size_type pos = s.find_first_of(delim, last_pos);
	while(string::npos != pos || string::npos != last_pos) {
		tokens.push_back(s.substr(last_pos, pos - last_pos));
		last_pos = s.find_first_not_of(delim, pos); 
		pos = s.find_first_of(delim, last_pos);
	}
}

vector<process_t> read_processes(const char* file_path)
{
	vector<process_t> process_queue;
	ifstream file(file_path);
	string str;
	while(getline(file, str)) {
		process_t new_process;
		stringstream ss(str);
		int service_num;
		char syntax;
		ss >> syntax >> new_process.process_id >> new_process.arrival_time >> service_num;
		for(int i = 0; i < service_num; i++) {	// read services sequence
			getline(file, str);
			str = str.erase(str.find_last_not_of(" \n\r\t") + 1);
			vector<string> tokens;
			split(str, tokens, " ");
			service_t ser(tokens[0], tokens[1]);
			new_process.service_seq.push_back(ser);
		}
		new_process.cur_service_idx = 0;
		new_process.cur_service_tick = 0;
		new_process.fb_level = 0;
		new_process.cur_service = new_process.service_seq[new_process.cur_service_idx];
		process_queue.push_back(new_process);
	}
	return process_queue;
}

// move the process at the front of q1 to the back of q2 (q1 head -> q2 tail)
int move_process_from(vector<process_t>& q1, vector<process_t>& q2)
{
	if(!q1.empty()) {
		process_t& tmp = q1.front();
		q2.push_back(tmp);
		q1.erase(q1.begin());
		return 1;
	}
	return 0;
}

int fcfs(vector<process_t> processes, const char* output_path) 
{
	vector<process_t> ready_queue;
	vector<process_t> processes_done;
    vector<process_t> d_block_queue; //block queue for disk i/o
    vector<process_t> k_block_queue; //block queue for keyboard interrupt
    vector<process_t> m_block_queue; //block queue for mutex

	int complete_num = 0;
	int dispatched_tick = 0;
	int cur_process_id = -1, prev_process_id = -1;

	// main loop
	for(int cur_tick = 0; cur_tick < MAX_LOOP; cur_tick++) {
		// long term scheduler
		for(int i = 0; i < processes.size(); i++) {
			if(processes[i].arrival_time == cur_tick) {		// process arrives at current tick
				ready_queue.push_back(processes[i]);
			}
		}
		// I/O device scheduling`
		if(!d_block_queue.empty()) {
			process_t& cur_io_process = d_block_queue.front();	// provide service to the first process in disk block queue
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// disk service is completed
				cur_io_process.proceed_to_next_service();
				move_process_from(d_block_queue, ready_queue);
			}
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
		}
        if(!k_block_queue.empty()) {
			process_t& cur_io_process = k_block_queue.front();	// provide service to the first process in keyboard block queue
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// keyboard service is completed
				cur_io_process.proceed_to_next_service();
				move_process_from(k_block_queue, ready_queue);
			}
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
		}
        if(!m_block_queue.empty()) {			
			if(mutex_lock == false) {	// mutex is unlocked
				move_process_from(m_block_queue, ready_queue);
				lock();
			}
		}


		// CPU scheduling
		if(ready_queue.empty()) {	// no process for scheduling
			prev_process_id = -1;	// reset the previous dispatched process ID to empty
		}
		else {
			process_t& cur_process = ready_queue.front();	// always dispatch the first process in ready queue
			cur_process_id = cur_process.process_id;
			if(cur_process_id != prev_process_id) {		// store the tick when current process is dispatched
				dispatched_tick = cur_tick;
			}
			if(cur_process.cur_service.type == "L") {		// next service is lock, lock the mutex if it is not being locked, blocked current process if it is already being locked
					if (mutex_lock == false){
						lock();
						cur_process.proceed_to_next_service();
					}
			}
			cur_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
			if(cur_process.cur_service_tick >= cur_process.cur_service.time_cost) {	// current service is completed
				cur_process.proceed_to_next_service();
				if(cur_process.cur_service.type == "L") {		// next service is lock, lock the mutex if it is not being locked, blocked current process if it is already being locked
					if (mutex_lock == false){
						lock(); // continue the process if mutex isn't locked
						cur_process.proceed_to_next_service();
					}
					else{
						cur_process.log_working(dispatched_tick, cur_tick + 1); // move process to block queue if the mutex is already locked
						move_process_from(ready_queue, m_block_queue);	
					}
				}
				else if(cur_process.cur_service.type == "U") { // next service is unlock, unlock the mutex
						unlock();
						cur_process.proceed_to_next_service();
				}
				bool process_completed = cur_process.check_end(); // bool for storing the vlaue of checkend() of current process
				if(process_completed) {		// the whole process is completed
					complete_num++;
					cur_process.log_working(dispatched_tick, cur_tick + 1);
					move_process_from(ready_queue, processes_done);		// remove current process from ready queue
				}			
				else if(cur_process.cur_service.type == "D") {		// next service is disk I/O, block current process
					cur_process.log_working(dispatched_tick, cur_tick + 1);
					move_process_from(ready_queue, d_block_queue);
				}
                else if(cur_process.cur_service.type == "K") {		// next service is keyboard I/O, block current process
					cur_process.log_working(dispatched_tick, cur_tick + 1);
					move_process_from(ready_queue, k_block_queue);
				}
			}
			prev_process_id = cur_process_id;	// log the previous dispatched process ID
		}
		if(complete_num == processes.size()) {	// all process completed
			break;
		}
	}
	write_file(processes_done, output_path);	// write output
	return 1;
}

int rr(vector<process_t> processes, const char* output_path)
{
	vector<process_t> ready_queue;
	vector<process_t> processes_done;
    vector<process_t> d_block_queue; //block queue for disk i/o
    vector<process_t> k_block_queue; //block queue for keyboard interrupt
    vector<process_t> m_block_queue; //block queue for mutex
	vector<process_t> temp_queue; //block queue for mutex
	int complete_num = 0;
	int dispatched_tick = 0;
	int qunatum_count = 0; // counter for clock interrupt
	int cur_process_id = -1, prev_process_id = -1;
	bool process_running = false; // store the bool value of whether a process is running in this program
	bool qunatum_tick = true; // bool value of deciding if the quantum_count will increment


	// main loop
	for(int cur_tick = 0; cur_tick < MAX_LOOP; cur_tick++) {
		// long term scheduler
		qunatum_tick = true;
		if (qunatum_count % time_quantum == 0 && qunatum_count != 0){ // process switch function for round robin
				if(ready_queue.size() >= 2 && process_running == true){
					process_t& front_process = ready_queue.front();
					front_process.log_working(dispatched_tick, cur_tick);		
					move_process_from(ready_queue, temp_queue);
					move_process_from(temp_queue, ready_queue);	
					dispatched_tick = cur_tick;
					cur_process_id = front_process.process_id;
					prev_process_id = cur_process_id;
					qunatum_count = 0;
				}		
		}
		
		for(int i = 0; i < processes.size(); i++) {
			if(processes[i].arrival_time == cur_tick) {		// process arrives at current tick
				ready_queue.push_back(processes[i]);
			}
		}
		// I/O device scheduling`
		if(!d_block_queue.empty()) {
			process_t& cur_io_process = d_block_queue.front();	// provide service to the first process in disk block queue
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// disk service is completed
				cur_io_process.proceed_to_next_service();
				move_process_from(d_block_queue, ready_queue);
			}
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
		}
        if(!k_block_queue.empty()) {
			process_t& cur_io_process = k_block_queue.front();	// provide service to the first process in keyboard block queue
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// keyboard service is completed
				cur_io_process.proceed_to_next_service();
				move_process_from(k_block_queue, ready_queue);
			}
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
		}
        if(!m_block_queue.empty()) {			
			if(mutex_lock == false) {	// mutex is unlocked
				move_process_from(m_block_queue, ready_queue);
				lock();
			}
		}
		
		// CPU scheduling
		if(ready_queue.empty()) {	// no process for scheduling
			prev_process_id = -1;	// reset the previous dispatched process ID to empty
		}
		else {
			process_t* cur_process = &ready_queue.front();	// always dispatch the first process in ready queue
			cur_process_id = cur_process->process_id;
			if(cur_process_id != prev_process_id) {		// store the tick when current process is dispatched
				dispatched_tick = cur_tick;
			}
			if(cur_process->cur_service.type == "L") {		// next service is lock, lock the mutex if it is not being locked, blocked current process if it is already being locked
				cur_process->proceed_to_next_service();
			}
			cur_process->cur_service_tick++;		// increment the num of ticks that have been spent on current service
			process_running = true;
			if(cur_process->cur_service_tick >= cur_process->cur_service.time_cost) {	// current service is completed
				cur_process->proceed_to_next_service();
				if(cur_process->cur_service.type == "L") {		// next service is lock, lock the mutex if it is not being locked, blocked current process if it is already being locked
					if (mutex_lock == false){ // continue the process if mutex isn't locked
						lock();
						cur_process->proceed_to_next_service();
						process_running = true;
					}
					else{
						cur_process->log_working(dispatched_tick, cur_tick + 1); // move process to block queue if the mutex is already locked
						move_process_from(ready_queue, m_block_queue);	
						process_running = false;
						qunatum_count = 0;
						qunatum_tick = false;
					}
				}
				else if(cur_process->cur_service.type == "U") { // next service is unlock, unlock the mutex
						unlock();
						cur_process->proceed_to_next_service();
				}
				bool process_completed = cur_process->check_end();	// bool for storing the vlaue of checkend() of current process
				if(process_completed) {		// the whole process is completed
					complete_num++;
					cur_process->log_working(dispatched_tick, cur_tick + 1);
					move_process_from(ready_queue, processes_done);		// remove current process from ready queue
					process_running = false;
					qunatum_count = 0;
					qunatum_tick = false;
				}			
				else if(cur_process->cur_service.type == "D") {		// next service is disk I/O, block current process
					cur_process->log_working(dispatched_tick, cur_tick + 1);
					move_process_from(ready_queue, d_block_queue);
					process_running = false;
					qunatum_count = 0;
					qunatum_tick = false;
				}
                else if(cur_process->cur_service.type == "K") {		// next service is keyboard I/O, block current process
					cur_process->log_working(dispatched_tick, cur_tick + 1);
					move_process_from(ready_queue, k_block_queue);
					process_running = false;
					qunatum_count = 0;
					qunatum_tick = false;
				}
			}
			prev_process_id = cur_process_id;	// log the previous dispatched process ID
		}
		if (qunatum_tick == true){
			qunatum_count++;
		}
		if(complete_num == processes.size()) {	// all process completed
			break;
		}				
	}
	write_file(processes_done, output_path);	// write output
	return 1;
}

int fb(vector<process_t> processes, const char* output_path)
{
	vector<process_t> l0_ready_queue; // ready queue for RQ0
	vector<process_t> l1_ready_queue; // ready queue for RQ1
	vector<process_t> l2_ready_queue; // ready queue for RQ2
	vector<process_t> processes_done;
    vector<process_t> d_block_queue; //block queue for disk i/o
    vector<process_t> k_block_queue; //block queue for keyboard interrupt
    vector<process_t> m_block_queue; //block queue for mutex
	vector<process_t> temp_queue; //block queue for mutex
	int complete_num = 0;
	int dispatched_tick = 0;
	int qunatum_count = 0; // counter for clock interrupt
	int cur_process_id = -1, prev_process_id = -1;
	int totalqueuesize = 0;
	int current_fb_level; // storing the level of cuurent process fb level
	bool process_running = false; // store the bool value of whether a process is running in this program
	bool qunatum_tick = true; // bool value of deciding if the quantum_count will increment


	// main loop
	for(int cur_tick = 0; cur_tick < MAX_LOOP; cur_tick++) {
		// long term scheduler
		qunatum_tick = true;
		totalqueuesize = l0_ready_queue.size() + l1_ready_queue.size() + l2_ready_queue.size();
		if (qunatum_count % time_quantum == 0 && qunatum_count != 0){ // process switch function for feedback schedule and increase the 
				if(totalqueuesize >= 2 && process_running == true){
					if (!l0_ready_queue.empty()){
						process_t& front_process = l0_ready_queue.front();
						front_process.lower_fb();
						front_process.log_working(dispatched_tick, cur_tick);		
						move_process_from(l0_ready_queue, temp_queue);
						move_process_from(temp_queue, l1_ready_queue);	
						
					}
					else if(!l1_ready_queue.empty()){
						process_t& front_process = l1_ready_queue.front();
						front_process.lower_fb();
						front_process.log_working(dispatched_tick, cur_tick);		
						move_process_from(l1_ready_queue, temp_queue);
						move_process_from(temp_queue, l2_ready_queue);	
						dispatched_tick = cur_tick;
						cur_process_id = front_process.process_id;
						prev_process_id = cur_process_id;
						qunatum_count = 0;
					}
					else if(!l2_ready_queue.empty()){
						process_t& front_process = l2_ready_queue.front();
						front_process.log_working(dispatched_tick, cur_tick);		
						move_process_from(l2_ready_queue, temp_queue);
						move_process_from(temp_queue, l2_ready_queue);	
						dispatched_tick = cur_tick;
						cur_process_id = front_process.process_id;
						prev_process_id = cur_process_id;
						qunatum_count = 0;
					}
				}		
		}
		
		for(int i = 0; i < processes.size(); i++) {
			if(processes[i].arrival_time == cur_tick) {		// process arrives at current tick
				l0_ready_queue.push_back(processes[i]);
			}
		}
		// I/O device scheduling`
		if(!d_block_queue.empty()) {
			process_t& cur_io_process = d_block_queue.front();	// provide service to the first process in disk block queue
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// disk service is completed
				cur_io_process.proceed_to_next_service();
				if (cur_io_process.get_fblevel() == 0){
					move_process_from(d_block_queue, l0_ready_queue);
				}
				else if (cur_io_process.get_fblevel() == 1){
					move_process_from(d_block_queue, l1_ready_queue);
				}
				else if (cur_io_process.get_fblevel() == 2){
					move_process_from(d_block_queue, l2_ready_queue);
				}
			}
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
		}
        if(!k_block_queue.empty()) {
			process_t& cur_io_process = k_block_queue.front();	// provide service to the first process in keyboard block queue
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// keyboard service is completed
				cur_io_process.proceed_to_next_service();
				if (cur_io_process.get_fblevel() == 0){
					move_process_from(k_block_queue, l0_ready_queue);
				}
				else if (cur_io_process.get_fblevel() == 1){
					move_process_from(k_block_queue, l1_ready_queue);
				}
				else if (cur_io_process.get_fblevel() == 2){
					move_process_from(k_block_queue, l2_ready_queue);
				}
			}
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
		}
        if(!m_block_queue.empty()) {			
			if(mutex_lock == false) {	// mutex is unlocked
				process_t& cur_io_process = m_block_queue.front();
				if (cur_io_process.get_fblevel() == 0){
					move_process_from(m_block_queue, l0_ready_queue);
					lock();
				}
				else if (cur_io_process.get_fblevel() == 1){
					move_process_from(m_block_queue, l1_ready_queue);
					lock();
				}
				else if (cur_io_process.get_fblevel() == 2){
					move_process_from(m_block_queue, l2_ready_queue);
					lock();
				}
			}
		}
		
		// CPU scheduling
		if(l0_ready_queue.empty() && l1_ready_queue.empty() && l2_ready_queue.empty()) {	// no process for scheduling
			prev_process_id = -1;	// reset the previous dispatched process ID to empty
		}
		else {
			process_t* cur_process;
			if(!l0_ready_queue.empty()){
				cur_process = &l0_ready_queue.front();	// always dispatch the first process in ready queue
			}
			else if(!l1_ready_queue.empty()){
				cur_process = &l1_ready_queue.front();
			}
			else{
				cur_process = &l2_ready_queue.front();
			}	
			current_fb_level = cur_process->get_fblevel();
			cur_process_id = cur_process->process_id;
			if(cur_process_id != prev_process_id) {		// store the tick when current process is dispatched
				dispatched_tick = cur_tick;
			}
			if(cur_process->cur_service.type == "L") {		// next service is lock, lock the mutex if it is not being locked, blocked current process if it is already being locked
				cur_process->proceed_to_next_service();
			}
			cur_process->cur_service_tick++;		// increment the num of ticks that have been spent on current service
			process_running = true;
			if(cur_process->cur_service_tick >= cur_process->cur_service.time_cost) {	// current service is completed
				cur_process->proceed_to_next_service();
				if(cur_process->cur_service.type == "L") {		// next service is lock, lock the mutex if it is not being locked, blocked current process if it is already being locked
					if (mutex_lock == false){ // continue the process if mutex isn't locked
						lock();
						cur_process->proceed_to_next_service();
						process_running = true;
					}
					else{
						cur_process->log_working(dispatched_tick, cur_tick + 1); // move process to block queue if the mutex is already locked
						if(current_fb_level == 0){
							move_process_from(l0_ready_queue, m_block_queue);
						}
						else if(current_fb_level == 1){
							move_process_from(l1_ready_queue, m_block_queue);	
						}
						else if(current_fb_level == 2){
							move_process_from(l2_ready_queue, m_block_queue);	
						}
						process_running = false;
						qunatum_count = 0;
						qunatum_tick = false;
					}
				}
				else if(cur_process->cur_service.type == "U") { // next service is unlock, unlock the mutex
						unlock();
						cur_process->proceed_to_next_service();
				}
				bool process_completed = cur_process->check_end();	// bool for storing the vlaue of checkend() of current process
				if(process_completed) {		// the whole process is completed
					complete_num++;
					cur_process->log_working(dispatched_tick, cur_tick + 1);
					if (current_fb_level == 0){
						move_process_from(l0_ready_queue, processes_done);		// remove current process from ready queue RQ1
					}
					else if(current_fb_level == 1){
						move_process_from(l1_ready_queue, processes_done);		// remove current process from ready queue RQ2
					}
					else if(current_fb_level == 2){
						move_process_from(l2_ready_queue, processes_done);		// remove current process from ready queue RQ3
					}
					process_running = false;
					qunatum_count = 0;
					qunatum_tick = false;
				}			
				else if(cur_process->cur_service.type == "D") {		// next service is disk I/O, block current process
					cur_process->log_working(dispatched_tick, cur_tick + 1);
					if (current_fb_level == 0){
						move_process_from(l0_ready_queue, d_block_queue);		// move current process from ready queue RQ1 to disk block queue
					}
					else if(current_fb_level == 1){
						move_process_from(l1_ready_queue, d_block_queue);		// move current process from ready queue RQ2 to disk block queue
					}
					else if(current_fb_level == 2){
						move_process_from(l2_ready_queue, d_block_queue);		// move current process from ready queue RQ3 to disk block queue
					}
					process_running = false;
					qunatum_count = 0;
					qunatum_tick = false;
				}
                else if(cur_process->cur_service.type == "K") {		// next service is keyboard I/O, block current process
					cur_process->log_working(dispatched_tick, cur_tick + 1);
					if (current_fb_level == 0){
						move_process_from(l0_ready_queue, k_block_queue);		// move current process from ready queue RQ1 to keyboard block queue
					}
					else if(current_fb_level == 1){
						move_process_from(l1_ready_queue, k_block_queue);		// move current process from ready queue RQ2 to keyboard block queue
					}
					else if(current_fb_level == 2){
						move_process_from(l2_ready_queue, k_block_queue);		// move current process from ready queue RQ3 to keyboard block queue
					}
					process_running = false;
					qunatum_count = 0;
					qunatum_tick = false;
				}
			}
			prev_process_id = cur_process_id;	// log the previous dispatched process ID
		}
		if (qunatum_tick == true){
			qunatum_count++;
		}
		if(complete_num == processes.size()) {	// all process completed
			break;
		}				
	}
	write_file(processes_done, output_path);	// write output
	return 1;
}

int main(int argc, char* argv[])
{
	if(argc != 4) {
		cout << "Incorrect inputs: too few arugments" << endl;
		return 0;
	}
	const char* process_path = argv[2];
	const char* output_path = argv[3];
	vector<process_t> process_queue = read_processes(process_path);
    if(!strcmp(argv[1], "FCFS")) {
		fcfs(process_queue, output_path);
	}
    else if(!strcmp(argv[1], "RR")){
        rr(process_queue, output_path);
    }
    else if(!strcmp(argv[1], "FB")){
        fb(process_queue, output_path);
    }
	return 0;
}
