#include "FileUtilities.h"

long FileUtilities::readlong(const char *filename, int pos){
  File file = SD.open(filename, FILE_READ);
  if(!file){
    return 0;
  }
  file.seek(pos);
  
  byte array[4];
  for(int j = 0; j < 4; j++){
    array[j] = file.read();
  }
 
  file.close();
  
  return *(reinterpret_cast<long*>(array));
}

byte FileUtilities::writelong(const char *filename, int pos, long val){
  byte* array = reinterpret_cast<byte *>(&val);
 
  File file = SD.open(filename, FILE_WRITE);
  if(!file){
    return 0;
  }
  file.seek(pos);
  byte nbB = file.write(array, sizeof(long));
  file.close();
  
  //On retourne le nombre d'octet écrit dans le fichier (11 caractères du long + 2 caractères de fin de ligne)
  return nbB;
}
