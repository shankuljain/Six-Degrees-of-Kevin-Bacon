
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include <stdlib.h>

using namespace std;

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
    const string actorFileName = directory + "/" + kActorFileName;
    const string movieFileName = directory + "/" + kMovieFileName;
    
    actorFile = acquireFileMap(actorFileName, actorInfo);
    movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
    return !( (actorInfo.fd == -1) ||
             (movieInfo.fd == -1) );
}

// you should be implementing these two methods right here...

 int cmpfn(const void *key, const void* add){
    string player = ((keyPair *) key)->player;
    string temp = ((char *)((keyPair *) key)->fileadd) + *(int*) add;
    return player.compare(temp);
}

int cmpfn2(const void *key, const void* add){
    char* temp_add = (char *)(((keyPair2 *) key)->fileadd) + *(int *) add;
    film temp;
    temp.title = temp_add;
    temp.year = 1900 + (int) *(temp_add + temp.title.length() + 1);
    
    film keymovie = ((keyPair2 *) key)->movie;
    
    if (keymovie == temp) {
        return 0;
    }else if(keymovie  < temp){
        return -1;
    }
    
    return 1;
}

bool imdb::getCredits(const string& player, vector<film>& films) const {
    int nActors =  *(int *)actorFile;
    
    keyPair key;
    key.player = player;
    key.fileadd = (void *)actorFile;
    
    int* offset =  (int *)bsearch(&key, (int *)actorFile+1, nActors, sizeof(int), cmpfn);
    
    if(offset!=NULL){
        char *actor = (char *)actorFile + *offset;
        
        int length = (int) strlen(actor);
        int temp = length - length%2;   //optimisation purpose
        
        short *nmovies = (short *)(actor+ 2 +temp);
        int *movie_offset_base = (int *)((char *)nmovies + 2 + temp%4);
        
       // cout<<*nmovies<<endl;
        
        for (int i=0; i<*nmovies; i++) {
            int *movie_offset = movie_offset_base + i;
            char *movie = (char *)movieFile + *movie_offset;
            //cout<<movie<<endl;
            film newmovie;
            newmovie.title= movie;
            newmovie.year = 1900 + (int) *(movie+strlen(movie)+1);
            films.push_back(newmovie);
            
            //cout<<newmovie.title<<" released in "<<newmovie.year<<endl;
        }
        
        return true;
    }
    
    return false;
}



bool imdb::getCast(const film& movie, vector<string>& players) const {
    int nmovies = *(int *)movieFile; //no of movies
    
    keyPair2 key;
    key.fileadd = (void *)movieFile;
    key.movie = movie;
    
    int *offset = (int *)bsearch(&key, (int*)movieFile+1, nmovies, sizeof(int), cmpfn2);
    
    char *moviebase =  (char *)movieFile + *offset; //ptr to moviename
    
    int length = (int) strlen(moviebase); // no of bytes required to store the moviename
    
    int temp = length + length%2;
  
    /*int year = 1900 + (int) *(moviebase + length + 1); //
    
    cout<<moviebase<<"released in" << year <<endl <<endl;*/
    
    
    short *nactors = (short *) (moviebase +temp + 2); //no of actors
    
    int *actor_offset_base = (int *) ((char *)nactors + 2 + temp%4);
    
    for (int i=0; i<*nactors; i++) {
        int *actor_offset = actor_offset_base + i;
        string player = (char *)actorFile + *actor_offset;
        players.push_back(player);
        //cout << player << endl;
    }
    
    return true;
}

imdb::~imdb()
{
    releaseFileMap(actorInfo);
    releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM..
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
    struct stat stats;
    stat(fileName.c_str(), &stats);
    info.fileSize = stats.st_size;
    info.fd = open(fileName.c_str(), O_RDONLY);
    return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
    if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
    if (info.fd != -1) close(info.fd);
}
