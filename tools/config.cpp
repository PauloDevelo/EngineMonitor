#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>


const int OLD_POS_AGE = 0;
const int OLD_POS_QTE = 12;
const int OLD_POS_TRIP = 24;
const int OLD_POS_TRIP_QTE = 36;
const int OLD_POS_TOT = 48;
const int OLD_POS_TOT_QTE = 60;
const int OLD_POS_TEMP_ALERT = 72;
const int OLD_POS_TEMP_FAN = 84;
const int OLD_POS_MAIN_QTE = 96;


const int POS_AGE = 0;
const int POS_QTE = 4;
const int POS_TRIP = 8;
const int POS_TRIP_QTE = 12;
const int POS_TOT = 16;
const int POS_TOT_QTE = 20;
const int POS_TEMP_ALERT = 24;
const int POS_TEMP_FAN = 28;
const int POS_MAIN_QTE = 32;


using namespace std;

bool seek(istream &file, int pos){
	int i = 0;
	char c;
	while(i < pos && file.get(c))i++;
	
	if(i < pos){
		cerr<<"Erreur dans le positionnement dans le fichier.\n";
		return false;
	}
	else{
		return true;
	}
}

void display(const char *filename, bool text = true){
	ifstream file(filename);
	if(!file){
		return;
	}
	
	
	char c;
	int nbCar = 0;
	if(text)cout<<"00[";
	
	while(file.get(c)){
		nbCar++;
		if(text){
			if(c == ' '){
				cout<<'_';
			}
			else if(c=='\n'){
				cout<<"]\n"<<nbCar<<'[';
			}
			else{
				cout<<c;
			}
		}
		else{
			cout<<int(c) << ' ';
		}
		
	}
	cout<<'\n';
}

int readlong(const char *filename, int pos){
	ifstream file(filename);
	  if(!file){
		return 0;
	  }
	  seek(file, pos);
  
	  char array[12];
	  for(int j = 0; j < 11; j++){
		file.get(array[j]);
		if(array[j] == ' '){
		  array[j] = 0;
		}
	  }
	  array[11] = 0;
	  int value = atoi(array);
	  
	  return value;
}

void writelong(const char *filename, int pos, int val){
	char* array = reinterpret_cast<char *>(&val);

	fstream file(filename, ios_base::binary|ios_base::in|ios_base::out);
	if(!file){
		return;
	}
	seek(file, pos);

	for(int i = 0; i < 4; i++){
		file.put(array[i]);
	}
}

int newReadlong(const char *filename, int pos){
	ifstream file(filename, ios_base::binary);
	if(!file){
		cerr<<"Erreur lors de l'ouverture du fichier\n";
		return 0;
	}
	
	
	if(seek(file, pos)){
		char array[4];
		for(int j = 0; j < 4; j++){
			if(!file.get(array[j])){
				cerr<<"Erreur lors de la lecture du fichier " << filename << ".\n";
				return -1;
			}
		}
		return *(reinterpret_cast<int*>(array));
	}
	else{
		
		return -1;
	}
}

void displayMenu(){
	cout<<"1:Afficher fichier de configuration\n";
	cout<<"2:Afficher ancien fichier de configuration\n";
	cout<<"3:Modifier fichier de config.\n";
	cout<<"Q:Quitter\n";
}

int main() {
	while(true){
		displayMenu();
		char c;
		cin>>c;
		
		if(c == 'Q' || c== 'q'){
			exit(0);
		}
		else if(c == '1'){
			cout<<"Nom du fichier : \n";
			string filename;
			cin>>filename;
			display(filename.c_str(), false);
			cout<<"AGE : "<< newReadlong(filename.c_str(), POS_AGE)<<'\n';
			cout<<"QTE : " << newReadlong(filename.c_str(), POS_QTE)<<'\n';
			cout<<"TRIP : " << newReadlong(filename.c_str(), POS_TRIP)<<'\n';
			cout<<"TRIP_QTE : " << newReadlong(filename.c_str(), POS_TRIP_QTE)<<'\n';
			cout<<"TOT : " << newReadlong(filename.c_str(), POS_TOT)<<'\n';
			cout<<"TOT_QTE : " << newReadlong(filename.c_str(), POS_TOT_QTE)<<'\n';
			cout<<"TEMP_ALERT : " << newReadlong(filename.c_str(), POS_TEMP_ALERT)<<'\n';
			cout<<"TEMP_FAN : " << newReadlong(filename.c_str(), POS_TEMP_FAN)<<'\n';
			cout<<"MAIN_QTE : " << newReadlong(filename.c_str(), POS_MAIN_QTE)<<"\n\n";
		}
		else if(c == '2'){
			cout<<"Nom du fichier : \n";
			string filename;
			cin>>filename;
			display(filename.c_str());
			cout<<"AGE : "<< readlong(filename.c_str(), OLD_POS_AGE)<<'\n';
			cout<<"QTE : " << readlong(filename.c_str(), OLD_POS_QTE)<<'\n';
			cout<<"TRIP : " << readlong(filename.c_str(), OLD_POS_TRIP)<<'\n';
			cout<<"TRIP_QTE : " << readlong(filename.c_str(), OLD_POS_TRIP_QTE)<<'\n';
			cout<<"TOT : " << readlong(filename.c_str(), OLD_POS_TOT)<<'\n';
			cout<<"TOT_QTE : " << readlong(filename.c_str(), OLD_POS_TOT_QTE)<<'\n';
			cout<<"TEMP_ALERT : " << readlong(filename.c_str(), OLD_POS_TEMP_ALERT)<<'\n';
			cout<<"TEMP_FAN : " << readlong(filename.c_str(), OLD_POS_TEMP_FAN)<<'\n';
			cout<<"MAIN_QTE : " << readlong(filename.c_str(), OLD_POS_MAIN_QTE)<<'\n'<<'\n';
		}
		else if(c == '3'){
			cout<<"Nom du fichier : \n";
			string filename;
			cin>>filename;
			
			cout<<POS_AGE << " : AGE\n";
			cout<<POS_QTE << " : QTE\n";
			cout<<POS_TRIP << " : TRIP\n";
			cout<<POS_TRIP_QTE << " : POS_TRIP_QTE\n";
			cout<<POS_TOT << " : TOT\n";
			cout<<POS_TOT_QTE << " : TOT_QTE\n";
			cout<<POS_TEMP_ALERT << " : TEMP_ALERT\n";
			cout<<POS_TEMP_FAN << " : TEMP_FAN\n";
			cout<<POS_MAIN_QTE<< " : MAIN_QTE\n";
			
			int pos;
			cin>>pos;
			
			cout<<"Valeur ?\n";
			long value;
			cin>>value;
			
			writelong(filename.c_str(), pos, value);
		}
	}
}
