/*
 * =====================================================================================
 * /
 * |     Filename:  sync_via_pen.cpp
 * |
 * |  Description:  
 * |
 * |      Version:  1.0
 * |      Created:  07/12/2015 04:19:56 PM
 * |     Revision:  none
 * |     Compiler:  gcc
 * `-. .--------------------
 *    Y
 *    ,,  ,---,
 *   (_,\/_\_/_\     Author:   Pablo González de Aledo (), pablo.aledo@gmail.com
 *     \.\_/_\_/>    Company:  Universidad de Cantabria
 *     '-'   '-'
 * =====================================================================================
 */

#include <string>
#include <map>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <set>

#define SIZE_STR 512

using namespace std;

FILE* log_file;
map<string, string> options;
set<string> ignores;
set<string> keep;
set<string> compress;
map<string,string> compress_md5s;
bool notify_end = false;

bool match_with_ignores(string line){

	for( set<string>::iterator it = ignores.begin(); it != ignores.end(); it++ ){

		FILE *fp;
		stringstream command;
		char ret[SIZE_STR];

		command << "echo \"" + line + "\" | egrep '" + *it + "'";

		fp = popen(command.str().c_str(), "r");

		if( fgets(ret,SIZE_STR, fp) != NULL ){
			pclose(fp);
			return true;
		} else {
			pclose(fp);
		}

	}

	return false;
	
}

void trim(char* line){
	if(line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;
	if(line[strlen(line)-1] == '/')  line[strlen(line)-1] = 0;
}

vector<string> tokenize(const string& str,const string& delimiters) {
	vector<string> tokens;
    	
	// skip delimiters at beginning.
    	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    	
	// find first "non-delimiter".
    	string::size_type pos = str.find_first_of(delimiters, lastPos);

    	while (string::npos != pos || string::npos != lastPos)
    	{
		// found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
	
		// skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
	
		// find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
    	}

	return tokens;
}

void start_logging(){
	log_file = fopen("spdata/log.log", "w");
}

void end_logging(){
	fclose(log_file);
}

void myReplace(std::string& str, const std::string& oldStr, const std::string& newStr) {
	size_t pos = 0;
	while((pos = str.find(oldStr, pos)) != std::string::npos){
		str.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
}

void escape(string& filename){
	myReplace(filename, "$", "\\$");
}

void escape_slash(string& filename){
	myReplace(filename, "/", "\\/");
}

void descape(string& filename){
	myReplace(filename, "\\$", "$");
}

string dirname(string file){
	FILE *fp;
	stringstream command;
	char ret[SIZE_STR];
	
	command << "dirname \"" << file << "\"";
	
	fp = popen(command.str().c_str(), "r");
	
	fgets(ret,SIZE_STR, fp);
	trim(ret);
	
	pclose(fp);

	string ret_s = string(ret);
	escape(ret_s);
	return ret_s;
}

void mkfolder(string path){
	system(("mkdir -p \"" + path + "\"").c_str());
}

void detox(string& filename){
	if(filename[0] == '.') filename[0] = '-';
	myReplace(filename, "%colon%", ":" );
	myReplace(filename, "%question%", "?" );
	myReplace(filename, "%dot%/", "./" );
	if(filename[0] == '-') filename[0] = '.';
}


void tox_and_detox(string& filename){
	if(filename[0] == '.'){
		filename[0] = '-';
		myReplace(filename, ":", "%colon%" );
		myReplace(filename, "?", "%question%" );
		myReplace(filename, "./", "%dot%/" );
		filename[0] = '.';
	} else {
		myReplace(filename, "%colon%", ":" );
		myReplace(filename, "%question%", "?" );
		myReplace(filename, "%dot%/", "./" );
	}
}

bool exist_local_file(string path){

	tox_and_detox(path);

	descape(path);
	return access(path.c_str(), F_OK ) != -1;

	//bool ret;
	//FILE *fp;
	//stringstream command;
	//char line[SIZE_STR];
	
	//command << "ls \"" << path << "\" 2>/dev/null";
	
	//fp = popen(command.str().c_str(), "r");
	
	//if(fgets(line,SIZE_STR, fp))
		//ret = true;
	//else 
		//ret = false;
	
	//pclose(fp);
	
	//return ret;
}

bool is_in_keep(string filename){
	return keep.find(filename) != keep.end();
}

bool is_in_compress(string filename, string path){
	for( set<string>::iterator it = compress.begin(); it != compress.end(); it++ ){
		string path_prefix = path + "/" + *it + "/";
		string filename_prefix = filename.substr(0, path_prefix.length());
		//printf("%s\n", (path_prefix).c_str() );
		//printf("%s\n", filename.substr(0, path_prefix.length()).c_str() );
		if(path_prefix == filename_prefix) return true;
	}

	return false;
}

int stoi(string str){
	int ret;
	sscanf(str.c_str(), "%d", &ret);
	return ret;
}

bool enough_space(){

	FILE *fp;
	stringstream command;
	char line[SIZE_STR];
	
	command << "df | grep Pablo | awk '{print $4}'";
	
	fp = popen(command.str().c_str(), "r");
	
	fgets(line,SIZE_STR, fp);
	trim(line);
		
	pclose(fp);

	return stoi(string(line)) > 50000;
	

}

void cpfile(string src, string dst){
	tox_and_detox(src);
	tox_and_detox(dst);
	if(match_with_ignores(src)) return;
	if(is_in_keep(dst)){
		if(options["fastkeep"] == "true" && exist_local_file(dst) ){
			cpfile(dst, "." + dst);
		}
		return;
	}
	string cpstr = (exist_local_file(dst))?"CP":"cp";
	int color = options["colors"]=="gr"? exist_local_file(dst)?31:32 : 32;
	if(src.find("spcompress") != string::npos && src[0] == '.' ) color=31;
	printf("\e[%dm %s \e[0m %s\n",color,cpstr.c_str(), src.c_str());
	fprintf(log_file, "\e[%dm %s \e[0m %s\n", color, cpstr.c_str(), src.c_str());
	if(options["dry_run"] == "true") return;
	string dir_name = dirname(dst); mkfolder(dir_name);
	system(("sudo rsync -a \"" + src + "\" \"" + dst + "\" 2>/dev/null").c_str());
	if(!enough_space()){
		printf("No enough space\n");
		system(("rm -f \"" + dst + "\"").c_str());

		string route = dst.substr(0, dst.find_last_of("/"));
		string filename = dst.substr(dst.find_last_of("/")+1);

		string rmcommand = "rm -f \'" + route + "/." + filename + "." + "\'*";
		//printf("rmcommand %s\n", rmcommand.c_str());
		system(rmcommand.c_str());
		//exit(0);



	}
}

void mvfile(string src, string dst){
	tox_and_detox(src);
	tox_and_detox(dst);
	if(match_with_ignores(src)) return;
	if(is_in_keep(dst)){
		if(options["fastkeep"] == "true" && exist_local_file(dst) ){
			cpfile(dst, "." + dst);
		}
		return;
	}
	string mvstr = (exist_local_file(dst))?"MV":"mv";
	int color = options["colors"]=="gr"? exist_local_file(dst)?31:32 : 33;
	if(src.find("spcompress") != string::npos && src[0] == '.') color=31;
	printf("\e[%dm %s \e[0m %s\n",color, mvstr.c_str(), src.c_str());
	fprintf(log_file, "\e[%dm %s \e[0m %s\n", color, mvstr.c_str(), src.c_str());
	if(options["dry_run"] == "true") return;
	string dir_name = dirname(dst); mkfolder(dir_name);

	if( exist_local_file("spdata/Trash")){
		mkfolder("spdata/Trash/" + dirname(dst));
		system(("mv \"" + dst + "\" \"spdata/Trash/" + dst + "\"").c_str());
	} else {
		system(("sudo rsync -a \"" + src + "\" \"" + dst + "\" && rm -f \"" + src + "\"").c_str());
	}
}

void rmfile(string file){
	tox_and_detox(file);
	if(match_with_ignores(file)) return;
	if(is_in_keep(file)){
		if(options["fastkeep"] == "true" && exist_local_file(file) ){
			cpfile(file, "." + file);
		}
		return;
	}
	printf("\e[31m rm \e[0m %s\n", file.c_str());
	fprintf(log_file, "\e[31m rm \e[0m %s\n", file.c_str());
	if(options["dry_run"] == "true") return;
	if( exist_local_file("spdata/Trash")){
		mkfolder("spdata/Trash/" + dirname(file));
		system(("mv \"" + file + "\" \"spdata/Trash/" + file + "\"").c_str());
	} else {
		system(("sudo rm -f \"" + file + "\"").c_str());
	}
}

string crc(string input){
	FILE *fp;
	stringstream command;
	char ret[SIZE_STR];
	
	command << "echo \"" + input + "\" | md5sum";
	
	fp = popen(command.str().c_str(), "r");
	
	fscanf(fp, "%s", ret);
	
	pclose(fp);

	return string(ret);
	
}

string unique_id(){

	if(options["unique_id"] != "") return options["unique_id"];

	FILE *fp;
	stringstream command;
	char ret[SIZE_STR];
	
	command << "( ifconfig eth0; ifconfig wlan0 ) 2>/dev/null | grep HWaddr | awk '{print $5}' | sed 's/://g'";

	
	fp = popen(command.str().c_str(), "r");
	
	fgets(ret,SIZE_STR, fp);
	trim(ret);
	
	pclose(fp);

	return string(ret);

}

map<string,string> load_md5(string filename){

	map<string, string> ret;

	if(!exist_local_file(filename.c_str())) return ret;

	FILE *file = fopen ( filename.c_str(), "r" );
	char line [ SIZE_STR ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);

		string md5 = string(line).substr(0, string(line).find(" "));
		string file = string(line).substr(string(line).find(" ") + 2);

		ret[file] = string(md5);
	}
	fclose ( file );

	return ret;
	
}

string md5_of_file(string filename){

	tox_and_detox(filename);

	stringstream command;
	char line[SIZE_STR];
	command.str("");
	command << "md5sum \"" << filename << "\"";

	FILE* fp = popen(command.str().c_str(), "r");
	fgets(line,SIZE_STR, fp);
	trim(line);
	pclose(fp);

	return string(line).substr(0, string(line).find(" "));
}

set<string> find_files(string path){
	FILE *fp;
	stringstream command;
	char line[SIZE_STR];
	set<string> files;

	command.str("");
	command << "find \"" << path << "\" -type f";
	fp = popen(command.str().c_str(), "r");
	while (fgets(line,SIZE_STR, fp) != NULL){
		trim(line);
		string line_s = string(line);
		escape(line_s);
		//if(match_with_ignores(line_s)) continue;
		files.insert(line_s);
	}
	pclose(fp);

	return files;
}

map<string,string> compute_md5_fast(string dir){

	FILE *fp;
	stringstream command;
	char line[SIZE_STR];
	set<string> recent_files;
	map<string, string> ret;
	
	//printf("Computing MD5's ... "); fflush(stdout);

	set<string> files = find_files(dir);

	map<string, string> md5s_prev = load_md5( "spdata/md5_remote_" + crc(dir) + "_" + unique_id() );

	command.str("");
	command << "find \"" << dir << "\" -type f -newer spdata/md5_remote_" << crc(dir) << "_" << unique_id();
	fp = popen(command.str().c_str(), "r");
	while (fgets(line,SIZE_STR, fp) != NULL){
		trim(line);
		recent_files.insert(string(line));
	}
	pclose(fp);


	for( set<string>::iterator it = files.begin(); it != files.end(); it++ ){
		if( recent_files.find(*it) != recent_files.end() ){
			ret[*it] = md5_of_file(*it);
		} else {
			ret[*it] = md5s_prev[*it];
		}

		if(ret[*it] == "")
			ret[*it] = md5_of_file(*it);
	}

	//printf("DONE\n");

	return ret;
	
}



void add_copy_to_filenames(string path, set<string>& filenames){

	FILE *fp;
	stringstream command;
	char line[SIZE_STR];
	set<string> recent_files;
	map<string, string> ret;
	

	set<string> files = find_files( "." + path);

	for( set<string>::iterator it = files.begin(); it != files.end(); it++ ){
		filenames.insert(it->substr(1));
	}
	
}

bool different_md5(string filename1, string filename2){
	if(!exist_local_file(filename1) || !exist_local_file(filename2)) return true;
	return md5_of_file(filename1) != md5_of_file(filename2);
}

map<string,string> compute_md5_slow(string dir){
	map<string, string> ret;
	
	//printf("Computing MD5's ... "); fflush(stdout);

	set<string> files = find_files(dir);

	for( set<string>::iterator it = files.begin(); it != files.end(); it++ ){
		string filename = *it;

		string md5 = md5_of_file(filename);

		ret[filename] = md5;

	}

	//printf("DONE\n");

	return ret;
}

void load_compress_md5s(string dir){

	map<string, string> ret;
	string filename = "spdata/md5_remote_" + crc(dir) + "_" + unique_id();
	if(!exist_local_file(filename.c_str())) return;
	FILE *file = fopen ( filename.c_str(), "r" );
	char line [ SIZE_STR ]; /* or other suitable maximum line size */
	compress_md5s.clear();
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);

		if(!strstr(line, "spcompress"))
			continue;

		if( exist_local_file(line) )
			continue;

		string md5 = string(line).substr(0, string(line).find(" "));
		string file = string(line).substr(string(line).find(" ") + 2);

		compress_md5s[file] = string(md5);
	}
	fclose ( file );
}

void add_to_md5(map<string, string>& map1, map<string, string> map2){

	for( map<string,string>::iterator it = map2.begin(); it != map2.end(); it++ ){
		map1[it->first] = it->second;
	}

}

map<string,string> compute_md5(string dir){

	load_compress_md5s(dir);

	if(options["fast_md5"] == "true" && exist_local_file("spdata/md5_remote_" + crc(dir) + "_" + unique_id()))
		return compute_md5_fast(dir);
	else
		return compute_md5_slow(dir);

}

set<string> get_filenames(map<string, string> map1, map<string, string> map2){

	set<string> ret;

	for( map<string,string>::iterator it = map1.begin(); it != map1.end(); it++ ){
		ret.insert(it->first);
	}
	for( map<string,string>::iterator it = map2.begin(); it != map2.end(); it++ ){
		ret.insert(it->first);
	}
	
	return ret;
}

string get_last_id(string path){

	FILE *file = fopen ( "spdata/latest", "r" );
	char line [ SIZE_STR ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		string line_s = string(line);
		string path_line = line_s.substr(0, line_s.find_last_of(" "));
		string id = line_s.substr(line_s.find_last_of(" ")+1);
		if(path_line == path ) return id;
	}
	fclose ( file );
	return "";

}

void set_my_md5_last(string path){

	if(options["dry_run"] == "true") return;

	if(!exist_local_file("spdata/latest")) system("touch spdata/latest");

	map<string, string> path_to_lastid;
	FILE *file = fopen ( "spdata/latest", "r" );
	char line [ SIZE_STR ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		string line_s = string(line);
		string path_line = line_s.substr(0, line_s.find_last_of(" "));
		string id = line_s.substr(line_s.find_last_of(" ")+1);
		path_to_lastid[path_line] = id;
	}
	fclose ( file );

	path_to_lastid[path] = unique_id();


	file = fopen("spdata/latest", "w");
	for( map<string,string>::iterator it = path_to_lastid.begin(); it != path_to_lastid.end(); it++ ){
		fprintf(file, "%s %s\n", it->first.c_str(), it->second.c_str());
	}
	fclose(file);

}

void add_to_md5_if_not_exists(map<string, string>& map1, map<string, string> map2){

	for( map<string,string>::iterator it = map2.begin(); it != map2.end(); it++ ){
		if(map1[it->first] == "")
			map1[it->first] = it->second;
	}

}

void dump_md5(map<string, string> md5s, string path){

	if(options["dry_run"] == "true") return;

	add_to_md5_if_not_exists(md5s, compress_md5s);

	FILE* file = fopen(("spdata/md5_remote_" + crc(path) + "_" + unique_id()).c_str(), "w");
	for( map<string,string>::iterator it = md5s.begin(); it != md5s.end(); it++ ){
		if(it->second != "")
			fprintf(file, "%s  %s\n", it->second.c_str(), it->first.c_str());
	}
	fclose(file);

	
}

map<string, string> load_retries(){


	map<string, string> ret;
	if(options["noretry"] == "true") return ret;
	if(!exist_local_file("spdata/retries")) return ret;
	FILE *file = fopen ( "spdata/retries", "r" );
	char line [ SIZE_STR ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		string line_s = string(line);
		string file = line_s.substr(0, line_s.find_last_of(" "));
		string id = line_s.substr(line_s.find_last_of(" ")+1);
		ret[file] = id;
	}
	fclose ( file );

	return ret;
	
}

void actualize_retries(string filename, map<string,string>& retries, string id){

	if(match_with_ignores(filename)) return;

	if(!exist_local_file(filename)){
		if(filename[0] == '.')
			retries[filename.substr(1)] = id;
		else
			retries[filename] = id;

	} else {
		if(filename[0] == '.')
			filename = filename.substr(1);
		if(retries.find(filename) != retries.end()){
			retries.erase(retries.find(filename));
		}
	}
}

bool is_in_path(string filename, string path){
	return filename.substr(0, path.length()) == path;
}


set<string> load_move_to_retry(){

	set<string> ret;
	if(!exist_local_file("spdata/move_to_retry")) return ret;

	FILE *file = fopen ( "spdata/move_to_retry", "r" );
	char line [ 1024 ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		ret.insert(string(line));
	}
	fclose ( file );

	return ret;
}

bool is_in_movetoretry(string filename){

	static bool movetoretry_loaded;
	static set<string> move_to_retry_list;
	if(!movetoretry_loaded){
		move_to_retry_list = load_move_to_retry();
		movetoretry_loaded = true;
	}

	return move_to_retry_list.find(filename) != move_to_retry_list.end();

}


void retry(map<string,string>& retries, string path){
	map<string,string> retries_at_end;
	if(options["noretry"] == "true") return;

	if(retries.size())
		printf("Retries...\n");

	string myid = unique_id();
	for( map<string,string>::iterator it = retries.begin(); it != retries.end(); it++ ){
		string filename = it->first;
		//if( !exist_local_file(filename) && it->second == myid){
		//	continue;
		//}

		if( is_in_movetoretry(filename) ) {
			retries_at_end[filename] = myid;
			continue;
		}


		if( is_in_path(filename, path) && it->second == myid){
			cpfile(filename, "." + filename);
			if(!exist_local_file("." + filename))
				retries_at_end[filename] = myid;

			continue;
		}

		retries_at_end[filename] = retries[filename];

	}

	retries = retries_at_end;
}

map<string, map<string, string> > load_md5s(string path, set<string> computers){ // file, idcomputer, md5

	map<string, map<string, string> > ret;

	for( set<string>::iterator it = computers.begin(); it != computers.end(); it++ ){

		map<string, string> md5s_remote_for_computer = load_md5( "./spdata/md5_remote_" + crc(path) + "_" + *it);
		for( map<string,string>::iterator it2 = md5s_remote_for_computer.begin(); it2 != md5s_remote_for_computer.end(); it2++ ){
			ret[it2->first][*it] = it2->second;
		}
		
	}

	return ret;

}

map<string, string> load_md5_latest(string path){
	return load_md5( "./spdata/md5_remote_" + crc(path) + "_" + get_last_id(path));
}

set<string> get_different_computers(){
	set<string> ret;
	FILE *fp;
	stringstream command;
	char line[SIZE_STR];
	
	command << "ls spdata/md5_remote_*";
	
	fp = popen(command.str().c_str(), "r");
	
	while (fgets(line,SIZE_STR, fp) != NULL){
		trim(line);
		ret.insert(tokenize(line, "_")[3]);
	}
	
	pclose(fp);

	return ret;
}

bool others_are_different(map<string, map<string,string> > md5_remotes,string file, set<string> computers) {
	set<string> different_md5s;

	map<string, string> id_to_md5 = md5_remotes[file];

	for( map<string,string>::iterator it = id_to_md5.begin(); it != id_to_md5.end(); it++ ){
		different_md5s.insert(it->second);
	}

	return different_md5s.size() > 1;
	
}

void save_retries(map<string, string> retries){

	if(options["dry_run"] == "true") return;
	if(options["noretry"] == "true") return;

	FILE* file = fopen("spdata/retries", "w");
	for( map<string,string>::iterator it = retries.begin(); it != retries.end(); it++ ){
		fprintf(file, "%s %s\n", it->first.c_str(), it->second.c_str());
	}
	fclose(file);

}

void add_md5s(map<string, map<string, string> >& md5_remotes, map<string, string> md5s_to_add, string id){
	for( map<string,string>::iterator it = md5s_to_add.begin(); it != md5s_to_add.end(); it++ ){
		md5_remotes[it->first][id] = it->second;
	}
}

void remove_id(set<string>& computers, string id){
	if(computers.find(id) != computers.end()){
		computers.erase(computers.find(id));
	}
}

bool find_one_different_of(map<string, string> md5s, set<string> computers, string md5){

	for( set<string>::iterator it = computers.begin(); it != computers.end(); it++ ){
		if(md5s[*it] != md5 ) return true;
	}
	
	return false;
}

void clean(string path){

	if(options["dry_run"] == "true") return;

	//system("rm -f spdata/keep");
	string path_escaped = path;
	escape_slash(path_escaped);
	system(("sed -i '/^" + path_escaped + "/d' spdata/keep 2>/dev/null").c_str());

	if(options["noclean"] == "true") return;

	set<string> computers                  = get_different_computers();
	map<string, map<string, string> > md5s = load_md5s(path, computers); // file, idcomputer, md5 

	FILE *fp;
	stringstream command;
	char ret[SIZE_STR];
	
	command << "find " << ".\"" + path + "\" -type f 2>/dev/null";
	
	fp = popen(command.str().c_str(), "r");
	
	while (fgets(ret,SIZE_STR, fp) != NULL){
		trim(ret);
		string ret_s = string(ret);
		string filename = ret_s.substr(1);
		escape(filename);
		tox_and_detox(filename);
		if( !find_one_different_of( md5s[filename],computers, md5s[filename][unique_id()]) ){
			rmfile("." + filename);
		}

	}
	
	pclose(fp);
	
	for ( unsigned int i = 0; i < 10; i++) {
		system(("find .\"" + path + "\" -mindepth 1 -type d -empty -delete 2>/dev/null").c_str());
	}

	command.str("");
	command << "find " << "\"" + path + "\" -mindepth 1 -type d -empty 2>/dev/null";
	
	fp = popen(command.str().c_str(), "r");
	
	while (fgets(ret,SIZE_STR, fp) != NULL){
		trim(ret);
		string folder = string(ret);
		if( !is_in_compress(folder, path) ){
			system(("rmdir \"" + folder + "\"").c_str());
		}

	}
	
	pclose(fp);

	system("find spdata -size 0 -not -name 'md5_remote_*' -delete");

}

string get_zenity_selection(){
	FILE *fp;
	stringstream command;
	char ret[SIZE_STR];
	
	command << "(echo start; echo end; echo drystart;) | zenity --list --column=option 2>/dev/null";
	
	fp = popen(command.str().c_str(), "r");
	
	fgets(ret,SIZE_STR, fp);
	trim(ret);
	
	return string(ret);

}

vector<string> get_paths_from_file(){
	vector<string> ret;
	FILE *file = fopen ( "spdata/paths", "r" );
	char line [SIZE_STR]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		if(line[0] != '#')
			ret.push_back(line);
	}
	fclose ( file );

	return ret;
}

bool is_in_retries(string filename, map<string, string> retries){
	return retries.find(filename) != retries.end();
}

bool is_in_retries(string filename){

	if(!exist_local_file("spdata/retries")) return false;
	FILE *file = fopen ( "spdata/retries", "r" );
	char line [ SIZE_STR ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		string line_s = string(line);
		string file_in_ret = line_s.substr(0, line_s.find_last_of(" "));
		if(file_in_ret == filename){
			fclose(file);
			return true;
		}
	}
	fclose ( file );

	return false;

}

void run_start_script(string path){
	if(options["dry_run"] == "true") return;
	if(!exist_local_file("spdata/start_script.sh")) return;
	system(("bash spdata/start_script.sh \"" + path + "\"").c_str());
}

void run_end_script(string path){
	if(options["dry_run"] == "true") return;
	if(!exist_local_file("spdata/end_script.sh")) return;
	system(("bash spdata/end_script.sh \"" + path + "\"").c_str());
}



void add_to_retry(string filename, map<string, string>& retries ){
	retries[filename] = unique_id();
}


typedef struct Unidir {
	string path;
	string from;
	string to;
} Unidir;

set<Unidir> unidirectionals;

inline bool operator<(const Unidir& lhs, const Unidir& rhs) {
	return lhs.path+lhs.from+lhs.to < rhs.path+rhs.from+rhs.to;
}



bool is_in_unidirectional(string path){
	for( set<Unidir>::iterator it = unidirectionals.begin(); it != unidirectionals.end(); it++ ){
		if(it->path == path) return true;
	}

	return false;
}

string unid_to(string path){
	for( set<Unidir>::iterator it = unidirectionals.begin(); it != unidirectionals.end(); it++ ){
		if(it->path == path) return it->to;
	}

}

string unid_from(string path){
	for( set<Unidir>::iterator it = unidirectionals.begin(); it != unidirectionals.end(); it++ ){
		if(it->path == path) return it->from;
	}

}

void remove_empty_folders(string dir){
	system(("find \"" + dir + "\" -empty -delete").c_str());
}

void start_working(string path){

	map<string, string> md5_local                 = compute_md5(path);
	map<string, string> md5_remote_latest         = load_md5_latest(path); // file, md5
	set<string> filenames                         = get_filenames(md5_local, md5_remote_latest);
	set<string> computers                         = get_different_computers(); remove_id(computers, unique_id()); remove_id(computers, get_last_id(path));
	map<string, map<string, string> > md5_remotes = load_md5s(path, computers); // file, idcomputer, md5
	string lastid                                 = get_last_id(path);
	string myid = unique_id();
	map<string, string> retries = load_retries(); // file, id

	if(is_in_unidirectional(path))
		add_copy_to_filenames(path, filenames);

	run_start_script(path);

	for( set<string>::iterator it = filenames.begin(); it != filenames.end(); it++ ){
		string filename              = *it;
		string remote_md5            = md5_remote_latest[filename];
		string local_md5             = md5_local[filename];
		bool   exist_local           = (local_md5 != "");
		bool   exist_copy            = exist_local_file("." + filename);
		bool   exist_remote          = (remote_md5 != "");
		bool   still_other_different = find_one_different_of( md5_remotes[filename],computers, remote_md5 );

		//if(filename.find("") != string::npos){
			//printf("%s %d %d %d %d\n",
					//filename.c_str(),
					//exist_local,
					//exist_copy,
					//exist_remote,
					//still_other_different
			//);
		//}
		if( is_in_unidirectional(path) && myid == unid_to(path) && !exist_local && exist_copy )                                   { mvfile("." + filename, filename); continue; }
		if( is_in_unidirectional(path) )                                                                                          { continue; }
		if( is_in_retries(filename, retries) )                                                                                    { continue; }
		if( is_in_compress(filename,path) )                                                                                       { continue; }
		if( is_in_movetoretry(filename) )                                                                                         { add_to_retry(filename, retries); continue; }
		if( exist_copy && remote_md5 == local_md5 && !still_other_different && myid != lastid)                                    { rmfile("." + filename); continue; }
		if( exist_remote && exist_local && remote_md5 == local_md5 )                                                              { continue; }
		if( !exist_remote && exist_local )                                                                                        { rmfile(filename); continue; }
		if( exist_remote && !exist_local && !exist_copy && filename.find("spcompress_") == string::npos)                          { actualize_retries("." + filename, retries, lastid); continue; }
		if( exist_remote && exist_local  && local_md5 != remote_md5 && !exist_copy )                                              { actualize_retries("." + filename, retries, lastid); continue; }
		if( exist_remote && !exist_local && exist_copy && !still_other_different && different_md5(filename, "." + filename) )     { actualize_retries("." + filename, retries, lastid); mvfile("." + filename, filename);continue; }
		if( exist_remote && !exist_local && exist_copy && still_other_different && different_md5(filename, "." + filename))       { actualize_retries("." + filename, retries, lastid); cpfile("." + filename, filename);continue; }
		if( remote_md5 != local_md5 && exist_copy && still_other_different && different_md5(filename, "." + filename))            { actualize_retries("." + filename, retries, lastid); cpfile("." + filename, filename);continue; }
		if( remote_md5 != local_md5 && exist_copy && !still_other_different && different_md5(filename, "." + filename))           { actualize_retries("." + filename, retries, lastid); mvfile("." + filename, filename);continue; }

	}

	retry(retries, path);
	save_retries(retries);
	dump_md5(compute_md5(path), path);
	if(!is_in_unidirectional(path))
		clean(path);
	if(is_in_unidirectional(path))
		remove_empty_folders("." + path);
}

long stol(string str){
	long ret;
	sscanf(str.c_str(), "%lu", &ret);
	return ret;
}

unsigned long get_epoch_last_end(string path){

	if(!exist_local_file("spdata/md5_remote_" + crc(path) + "_" + unique_id()))
		return 0;

	FILE *fp;
	stringstream command;
	char ret[SIZE_STR];
	
	command << "stat --format=%Y spdata/md5_remote_" + crc(path) + "_" + unique_id();
	
	fp = popen(command.str().c_str(), "r");
	
	fgets(ret,SIZE_STR, fp);
	ret[strlen(ret)-1] = 0;
	
	pclose(fp);

	return stol(string(ret));
	
}

bool modified_after_epoch(string filename, unsigned long time ){
	FILE *fp;
	stringstream command;
	char mtime[SIZE_STR];
	
	command << "stat --format=%Y.%Z \"" << filename << "\"";
	
	fp = popen(command.str().c_str(), "r");
	
	fgets(mtime,SIZE_STR, fp);
	mtime[strlen(mtime)-1] = 0;
	
	pclose(fp);

	long mtime1 = stol(tokenize(string(mtime), ".")[0]);
	long mtime2 = stol(tokenize(string(mtime), ".")[1]);


	return mtime1 > time || mtime2 > time;
}

map<string, string> filter_retries(map<string, string> retries, string path, string myid){
	map<string, string> ret;
	for( map<string,string>::iterator it = retries.begin(); it != retries.end(); it++ ){
		if(is_in_path(it->first, path) && it->second == myid){
			ret[it->first] = it->second;
		}
	}

	return ret;
	
}

void fastrm(string path){

	if(options["fastrm"] == "false") return;

	FILE *fp;
	stringstream command;
	char ret[SIZE_STR];
	
	command << "find " << ".\"" + path + "\" -type f 2>/dev/null";
	
	fp = popen(command.str().c_str(), "r");
	
	while (fgets(ret,SIZE_STR, fp) != NULL){
		trim(ret);
		string ret_s = string(ret);
		string filename = ret_s.substr(1);
		if( !exist_local_file(filename) && filename.find("spcompress_") == string::npos ){
			rmfile("." + filename);
		}

	}
	
	pclose(fp);

}

string ltos(unsigned long i){
	stringstream i_ss;
	i_ss << i;
	return i_ss.str();
}

void set_time(string file, unsigned long time){
	if(!exist_local_file(file)) return;
	system(("touch -d '@" + ltos(time) + "' " + file).c_str());
}


void end_working(string path){

	unsigned long start_time = time(NULL);
	map<string, string> retries = load_retries();
	map<string, string> md5_local = compute_md5(path);
	map<string, string> md5_remote = load_md5("spdata/md5_remote_" + crc(path) + "_" + get_last_id(path));
	unsigned long epoch_last_end = get_epoch_last_end(path);
	map<string, string> empty;
	string myid = unique_id();
	set<string> computers                         = get_different_computers(); remove_id(computers, unique_id()); remove_id(computers, get_last_id(path));
	map<string, map<string, string> > md5_remotes = load_md5s(path, computers); // file, idcomputer, md5 

	if(!is_in_unidirectional(path))
		fastrm(path);

	run_end_script(path);

	set<string> filenames = get_filenames(md5_local, empty);

	for( set<string>::iterator it = filenames.begin(); it != filenames.end(); it++ ){

		string filename     = *it;
		string remote_md5   = md5_remote[filename];
		string local_md5    = md5_local[filename];
		bool   exist_local  = (local_md5 != "");
		bool   exist_copy   = exist_local_file("." + filename);
		bool   exist_remote = (remote_md5 != "");
		bool   still_other_different = find_one_different_of( md5_remotes[filename],computers, local_md5);

		//if(filename.find("") != string::npos){
			//printf("%s %d %d %d %d\n",
					//filename.c_str(),
					//exist_local,
					//exist_copy,
					//exist_remote,
					//still_other_different
			//);
		//}
		
		
		if( is_in_unidirectional(path) && myid == unid_from(path) && exist_local && !exist_copy )      { mvfile(filename, "." + filename); continue; }
		if( is_in_unidirectional(path) )                                                               { continue; }
		if( is_in_retries(filename, retries) )                                                         { continue; }
		if( is_in_compress(filename,path) )                                                            { continue; }
		if( still_other_different && modified_after_epoch(filename, epoch_last_end))                   { cpfile(filename, "." + filename);actualize_retries("." + filename, retries, myid); continue; }
		if( still_other_different && !exist_copy )                                                     { cpfile(filename, "." + filename);actualize_retries("." + filename, retries, myid); continue; }
		if( !exist_remote && modified_after_epoch(filename, epoch_last_end))                           { cpfile(filename, "." + filename);actualize_retries("." + filename, retries, myid); continue; }
		if( !exist_remote && !exist_copy )                                                             { cpfile(filename, "." + filename);actualize_retries("." + filename, retries, myid); continue; }
		if( exist_remote && local_md5 != remote_md5 && modified_after_epoch(filename, epoch_last_end)) { cpfile(filename, "." + filename);actualize_retries("." + filename, retries, myid); continue; }

	}

	if(filter_retries(retries, path, myid).size() && options["dry_run"] == "false"){
		printf("\e[33m Warning: Not all files could be copied !! \e[0m\n");
	}

	save_retries(retries);
	dump_md5(md5_local, path);
	set_time( "spdata/md5_remote_" + crc(path) + "_" + get_last_id(path), start_time);
	if(!is_in_unidirectional(path))
		clean(path);
}

void setup(string path){
	char input[SIZE_STR] = {0};
	printf("Have you already copied the files to the destination(s)? [Y/n] ");
	//scanf("%s", input);
	fgets(input, SIZE_STR, stdin);
	input[strlen(input)-1] = 0;

	if(!strcmp(input, "Y") || !strcmp(input, "") || !strcmp(input, "y")){
		dump_md5(compute_md5(path), path);
	} else if (!strcmp(input, "n")){
		system("mkdir -p spdata");
		system(("mkdir -p .\"" + path + "\"").c_str());
		system(("rsync -vai --delete \"" + path + "/\"" + " .\"" + path + "\"").c_str());
		dump_md5(compute_md5(path), path);
	}

}

void load_config(){
	options["fast_md5"] = "true";
	options["dry_run"] = "false";
	options["noretry"] = "false";
	options["noclean"] = "false";
	options["fastkeep"] = "true";
	options["fastrm"] = "true";
	options["colors"] = "gr";
	//if(!exist_local_file("spdata/config")) return;
	//FILE *file = fopen ( "spdata/config", "r" );
	//char line [ SIZE_STR ]; [> or other suitable maximum line size <]
	
	//while ( fgets ( line, sizeof(line), file ) != NULL ){
		//trim(line);
		//string line_s = string(line);
		//options[tokenize(line_s, ":=")[0] ] = tokenize(line_s,":=")[1];
	//}
	//fclose ( file );
	
}

void load_ignores(){
	if(!exist_local_file("spdata/ignores")) return;

	FILE *file = fopen ( "spdata/ignores", "r" );
	char line [ SIZE_STR ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		ignores.insert(string(line));
		
	}
	fclose ( file );

}

void load_keep(){
	if(!exist_local_file("spdata/keep")) return;

	FILE *file = fopen ( "spdata/keep", "r" );
	char line [ SIZE_STR ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		string line_s = string(line);
		detox(line_s);
		keep.insert(line_s);
		
	}
	fclose ( file );

}

void load_compress(){
	if(!exist_local_file("spdata/compress")) return;

	FILE *file = fopen ( "spdata/compress", "r" );
	char line [ SIZE_STR ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		compress.insert(string(line));
		
	}
	fclose ( file );

}

void check_log(){

	if(!exist_local_file("spdata/log.log")) return;
	FILE *file = fopen ( "spdata/log.log", "r" );
	char line [ 1024 ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		string line_s = string(line);

		if(line_s.substr(0,13) != "\e[32m cp \e[0m" && 
		   line_s.substr(0,13) != "\e[32m CP \e[0m" &&
		   line_s.substr(0,13) != "\e[31m cp \e[0m" &&
		   line_s.substr(0,13) != "\e[31m CP \e[0m" &&
		   line_s.substr(0,13) != "\e[32m mv \e[0m" &&
		   line_s.substr(0,13) != "\e[32m MV \e[0m" &&
		   line_s.substr(0,13) != "\e[31m MV \e[0m" &&
		   line_s.substr(0,13) != "\e[33m mv \e[0m" &&
		   line_s.substr(0,13) != "\e[31m mv \e[0m" &&
		   line_s.substr(0,13) != "\e[33m MV \e[0m" &&
		   line_s.substr(0,13) != "\e[31m rm \e[0m" ){
			printf("\e[33m Incorrect log file\e[0m %s\n", line);
			exit(-1);
		}
	}
	fclose ( file );

	
}

bool something_modified_after(string path, string prefix, string file){

	FILE *fp;
	stringstream command;
	char ret[SIZE_STR];
	
	command << "find " << path << "/" << prefix << " -newer " << file << " 2>/dev/null";
	
	fp = popen(command.str().c_str(), "r");
	
	if(fgets(ret,SIZE_STR, fp) != NULL){
		pclose(fp);
		return true;
	} else {
		pclose(fp);
		return false;
	}
	
	
}

bool inlist(string compress, string path){

	FILE *fp;
	stringstream command;
	char ret[SIZE_STR];

	string compress_prefix = compress;
	string filename = "spcompress_" + compress_prefix + ".tar.bz2";
	myReplace(filename, "/", "_");


	command << "cat spdata/md5_remote_" + crc(path) + "_" + unique_id() + " | grep " + filename;

	fp = popen(command.str().c_str(), "r");

	if(fgets(ret,SIZE_STR, fp) == NULL){
		pclose(fp);
		return false;
	} else {
		pclose(fp);
		return true;
	}

			
}

void do_compress(string path){

	for( set<string>::iterator it = compress.begin(); it != compress.end(); it++ ){

		string compress_prefix = *it;
		stringstream command;
		string filename = "spcompress_" + compress_prefix + ".tar.bz2";
		myReplace(filename, "/", "_");

		bool force_compress = false;
		if( exist_local_file(path + "/" + (*it) ) && !inlist(*it, path) )
			force_compress = true;	

		if(is_in_retries( path + "/" + filename))
			force_compress = true;	

		if(is_in_keep( path + "/" + filename))
			force_compress = true;	

		set<string> computers                         = get_different_computers();
		map<string, map<string, string> > md5_remotes = load_md5s(path, computers); // file, idcomputer, md5_remotes
		string remote_md5 = md5_remotes[filename][*(computers.begin())];
		bool   still_other_different = find_one_different_of( md5_remotes[filename],computers, remote_md5 );
		if(still_other_different)
			force_compress = true;

		if(!force_compress && !something_modified_after(path, *it, "spdata/md5_remote_" + crc(path)+ "_" + unique_id() )){
			continue;
		}
		command << "cd " << path << ";";
		command << "tar -cjf " << filename << " " << compress_prefix << ";";
		system(command.str().c_str());
	}
}

void do_uncompress(string path){

	for( set<string>::iterator it = compress.begin(); it != compress.end(); it++ ){
		string compress_prefix = *it;
		stringstream command;
		string filename = "spcompress_" + compress_prefix + ".tar.bz2";
		myReplace(filename, "/", "_");
		command << "cd " << path << ";";
		command << "[ -e " << filename << " ] && ";
		command << "rm -rf " << compress_prefix << " && ";
		command << "tar -xjf " << filename << " && ";
		command << "rm " << filename;
		system(command.str().c_str());
	}
}

void do_removecompressed(string path){
	for( set<string>::iterator it = compress.begin(); it != compress.end(); it++ ){
		string compress_prefix = *it;
		stringstream command;
		string filename = "spcompress_" + compress_prefix + ".tar.bz2";
		myReplace(filename, "/", "_");
		command << "cd " << path << ";";
		command << "rm -f " << filename;
		system(command.str().c_str());
	}
}

void move_to_retry(){
	stringstream command;
	command << "cat " << "./spdata/move_to_retry" << " 2>/dev/null ";
	command << "| sed 's/$/" << " " << unique_id() << "/g' ";
	command << ">> ./spdata/retries";
	command << " && rm -f ./spdata/move_to_retry";
	system(command.str().c_str());
}

void show_retries(){
	if(options["show_retries"] == "false") return;
	if(!exist_local_file("spdata/retries")) return;
	printf("\e[31m Retries \e[0m\n");
	system("cat spdata/retries");
}

set<string> exclude_once;

void load_exclude_once(){
	if(!exist_local_file("spdata/exclude_once")) return;
	FILE *file = fopen ( "spdata/exclude_once", "r" );
	char line [ 1024 ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		exclude_once.insert(line);
	}
	fclose ( file );
	
}

void rm_exclude_once(){
	if(options["dry_run"] == "true") return;
	system("rm -f spdata/exclude_once");
}

bool is_in_exclude_once(string path){
	return exclude_once.find(path) != exclude_once.end();
}

string equiv(string file){
	if(file[0] == '.') return file.substr(1);
	else if(file[0] == '/') return "." + file;
}

string spcequiv(string file){
	myReplace(file, "spcompress_", "");
	myReplace(file, "_", "/");
	myReplace(file, ".tar.bz2", "");
	return file;
}



void load_unidirectional(){

	if(!exist_local_file("spdata/unidirectional")) return;
	
	FILE *file = fopen ( "spdata/unidirectional", "r" );
	char line [ 128 ]; /* or other suitable maximum line size */
	
	while ( fgets ( line, sizeof(line), file ) != NULL ){
		trim(line);
		string line_s = string(line);
		string path = tokenize(line_s, " ")[0];
		string from = tokenize(line_s, " ")[1];
		string to   = tokenize(line_s, " ")[2];
		Unidir unidir; unidir.path=path; unidir.from=from; unidir.to=to;
		unidirectionals.insert(unidir);
	}

	fclose ( file );
}

bool is_encrypted(string path){
	set<string> files = find_files(path);
	for( set<string>::iterator it = files.begin(); it != files.end(); it++ ){
		if( (*it).find("img.img") != string::npos ) return true;
	}
	return false;
}

int main(int argc, const char *argv[]){

	//string escaped = "/media/disk";
	//escape_slash(escaped);
	//printf("escapeslash %s\n", escaped.c_str());
	//exit(0);
	
	
	
	if(string(argv[1]) == "meld"){
		string file = string(argv[2]);
		if(file.find("spcompress") == string::npos){
			system(("meld " + file + " " + equiv(file)).c_str());
		} else {

			string route = file.substr(1, file.find_last_of("/"));
			string filename = file.substr(file.find_last_of("/")+1);
			string route2 = filename;
			myReplace(route2,"spcompress_", "");
			myReplace(route2,".tar.bz2", "");
			myReplace(route2, "_","/");
			//printf("%s %s %s\n", route.c_str(), filename.c_str(), route2.c_str());
			stringstream command;
			command << "PTH=$PWD;";
			command << "mkdir /tmp/spcompress 2>/dev/null; rm -rf /tmp/spcompress/*;";
			command << "cd /tmp/spcompress/; tar -xjf $PTH/" << "/" << file << ";";
			command << "meld " << route2 << " " << route << "/" << route2 << ";";
			system( command.str().c_str() );

		}
		exit(0);
	}

	
	

	check_log();
	start_logging();
	load_config();
	load_ignores();
	load_keep();
	load_compress();
	load_exclude_once();
	load_unidirectional();

	string selection;
	vector<string> paths;

	if(argc == 1){
		selection = get_zenity_selection();
		paths = get_paths_from_file();
		notify_end = true;
	} 

	if(argc == 2){
		selection = string(argv[1]);
		paths = get_paths_from_file();
	}

	if(argc == 3){
		selection = string(argv[1]);
		paths.push_back(argv[2]);

	}

	if(selection == "reset"){
		rmfile("./spdata/retries");
		rmfile("./spdata/keep");
		rmfile("./spdata/move_to_retry");
		exit(0);
	}

	for( vector<string>::iterator it = paths.begin(); it != paths.end(); it++ ){
		string path=*it;

		if(is_in_exclude_once(path)) continue;
		if(is_encrypted(path)){
			printf("Path \e[34m%s\e[0m\n", path.c_str());
			printf("\e[33m Is encrypted, skipping \e[0m\n");
			continue;
		}

		if(exist_local_file(*it)){
			printf("Path \e[34m%s\e[0m\n", path.c_str());
		} else {
			printf("\e[31mSkipping %s\e[0m\n", path.c_str());
			continue;
		}

		if(selection == "start"){
			do_compress(path);
			start_working(path);
			do_uncompress(path);
		} else if(selection == "end"){
			move_to_retry();
			do_compress(path);
			end_working(path);
			do_removecompressed(path);
			system("rm -f spdata/move_to_retry");
		} else if(selection == "drystart"){
			options["dry_run"] = "true";
			do_compress(path);
			start_working(path);
			do_uncompress(path);
		} else if(selection == "dryend"){
			options["dry_run"] = "true";
			do_compress(path);
			end_working(path);
			do_uncompress(path);
		} else if(selection == "setup"){
			setup(path);
		} else if(selection == "md5s"){
			dump_md5(compute_md5(path), path);
		} else if(selection == "retry"){
			map<string, string> retries = load_retries(); // file, id
			retry(retries, path);
			save_retries(retries);
		}

		set_my_md5_last(path);
	}

	rm_exclude_once();

	end_logging();
	show_retries();

	if(notify_end){
		system("zenity --notification --text 'sync_via_pen finished'");
	}

	return 0;



}

