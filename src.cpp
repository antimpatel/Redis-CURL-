#include <iostream>
#include <string>
#include<fstream>
#include <curl/curl.h>
#include "lib/hiredis.h"
using namespace std;

string data; //will hold the url's contents

size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up)
{  
    for (int c = 0; c<size*nmemb; c++)
    {
        data.push_back(buf[c]);
    }
    return size*nmemb; //tell curl how many bytes we handled
}
/*
this function gets data and then parse the json data
in this format-travellercompany::departure time::arrivaltime::fare
*/
string parse(ifstream &file,string content,size_t found){
	string temp="";int j=found+14,k;bool flag;
	while(j<content.size())
	{
		if(content[j]!='"')temp+=content[j++];
		if(content[j]=='"'){flag=0;break;}
	}
	temp+=" ";
	while(file>>content)//this parse traveller company
	{
		if(content.find('"')==string::npos)
		{
			temp+=content;temp+=" ";
		}		
		else
		{
			k=0;
			while(content[k]!='"'){temp+=content[k++];}			
			break;
		} 
	}
	temp+="::DEPARTURE::";
	k = content.find("DepartureTime")+16;
	while(content[k]!='"')//this loop parse departure time
	{
	temp+=content[k++];
	}
	temp+="::ARRIVS ON::";
	k = content.find("ArrivalTime")+14;
	while(content[k]!='"')//this parse arrival time
	{
	temp+=content[k++];
	}
	temp+="::FARE::";
	k = content.find("Fare")+6;
	while(content[k]!='"')//this parse fare
	{
	temp+=content[k++];
	}
	return temp;
}
/*
this reply with url from where we have to get data
*/
string geturl(string date,string city1,string city2){
	string s1="http://www.travelyaari.com//api/search/?mode=oneway&departDate=";
	string s2="&fromCity=";
	string s3="&toCity=";
	string s4="&pickups=1";
	string final=s1+date+s2+city1+s3+city2+s4;
	return final;
}
/*
this function gets data from travelyaari 
*/
void Curl_setup(string final){
	CURL* curl; //our curl object
    	curl_global_init(CURL_GLOBAL_ALL); //pretty obvious
    	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
    	curl_easy_setopt(curl,CURLOPT_URL, final.c_str());
    	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);	
    	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); //tell curl to output its progress
    	curl_easy_perform(curl);
	cout << endl << data << endl;
	curl_easy_cleanup(curl);
    	curl_global_cleanup();
}
/*
check whether data already availble in redis server or not
if yes then print to console
*/
bool check(string t,redisReply *reply,redisContext *c){
	const char*tt=t.c_str();
	reply=(redisReply*)redisCommand(c,"LRANGE %s 0 -1",tt);
	if(reply->type==REDIS_REPLY_ARRAY&&reply->elements>0){
		fclose(stdout);
	freopen ("/dev/tty", "a", stdout);
	//cout<<"intrrr"<<endl;;
	for(int i=0;i<reply->elements;i++)	
		cout<<reply->element[i]->str<<endl;
	
		return 1;
	}
return 0;
}


/*
this function  inserts data into the redis server after parsing the json file
data parsed in foemat travellername::departure time::arrivaltime::fare
*/
void Dump_to_redis(redisReply *reply,redisContext *c,string final)
{
	ifstream file("fi.txt");
	string content,str;
	int i=0;
	while( file>>content){
		std::size_t found = content.find("CompanyName");
  		if (found!=std::string::npos)
		{
		 	const char*myid=final.c_str();
			string parsed_string=parse(file,content,found);
			const char*value=parsed_string.c_str();
		 	reply=(redisReply*)redisCommand(c, "LPUSH %s %s" ,myid,value);
			if(!reply)printf("ERROR\n");
	
		
		}
 	}

}
	/*****end of dump_to_redis******/

int main()
{
	redisReply *reply;
 	redisContext *c = redisConnect("127.0.0.1", 6379);//connect to redis server localhost
	if (c->err) {
        	printf("Error: %s\n", c->errstr);
    	}else{
        	printf("Connection Made! \n");
    	}
	
	cout<<"enter date(DD-MM-YYYY) sorce city, dest city\n";
	freopen("fi.txt","w",stdout);
	string city1,city2,date;
	cin>>date>>city1>>city2;
	
	if(check(city1+city2,reply,c))// check whether data is available or not
	{
	return 0;
	}
	 
	string Url=geturl(date,city1,city2);
	 
	Curl_setup(Url);//get data from travelyaari
	 	
	fclose(stdout);
	freopen ("/dev/tty", "a", stdout);
   	Dump_to_redis(reply,c,city1+city2);//insert data to server
    return 0;
}
