#include "layer.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

/*! Load a layer from the given file */
void load_layer(
    const char *srcFile,
    uint16_t &nIn,
    uint16_t &nOut,
    std::vector<synapse_t> &synapses
){
    FILE *hFile=stdin;
    
    if(strcmp(srcFile,"<STDIN>")){
        hFile=fopen(srcFile, "rb");
        if(hFile==NULL){
            fprintf(stderr, "ERROR : Couldn't open input file '%s'.\n", srcFile);
            exit(1);
        }
    }
    
    if(1!=fread(&nIn, 2, 1, hFile)){
        fprintf(stderr, "ERROR : Couldn't read nIn from '%s'.\n", srcFile);
        exit(1);
    }
    if(1!=fread(&nOut, 2, 1, hFile)){
        fprintf(stderr, "ERROR : Couldn't read nOut from '%s'.\n", srcFile);
        exit(1);
    }
    
    uint32_t nSynapses=0;
    if(1!=fread(&nSynapses, 4, 1, hFile)){
        fprintf(stderr, "ERROR : Couldn't read nSynapses from '%s'.\n", srcFile);
        exit(1);
    }
    
    synapses.resize(nSynapses);
    
    if(sizeof(synapse_t)!=8){
        fprintf(stderr, "ERROR : sizeof(synapse_t)!=8\n");
        exit(1);
    }
    
    if(nSynapses!=fread(&synapses[0], sizeof(synapse_t), nSynapses, hFile)){
        fprintf(stderr, "ERROR : Couldn't read %u synapses from '%s'.\n", nSynapses, srcFile);
        exit(1);
    }
    
    for(unsigned i=0; i<synapses.size(); i++){
        if( (synapses[i].src >= nIn) || (synapses[i].dst >= nOut) ){
            fprintf(stderr, "ERROR : File '%s' is corrupt at synapse %u.\n", srcFile, i);
            exit(1);
        }
    }
    
    if(strcmp(srcFile,"<STDIN>")){
        fclose(hFile);
    }
}

/*! Save a layer to the given file */
void save_layer(
    const char *dstFile,
    uint16_t nIn,
    uint16_t nOut,
    const std::vector<synapse_t> &synapses
){
    for(unsigned i=0; i<synapses.size(); i++){
        if( (synapses[i].src >= nIn) || (synapses[i].dst >= nOut) ){
            fprintf(stderr, "ERROR : Synapses being written to '%s' are corrupt at synapse %u.\n", dstFile, i);
            exit(1);
        }
    }
    
    FILE *hFile=stdout;
    
    if(strcmp(dstFile,"<STDOUT>")){
        hFile=fopen(dstFile, "wb");
        if(hFile==NULL){
            fprintf(stderr, "ERROR : Couldn't open output file '%s'.\n", dstFile);
            exit(1);
        }
    }
    
    if(1!=fwrite(&nIn, 2, 1, hFile)){
        fprintf(stderr, "ERROR : Couldn't write nIn to '%s'.\n", dstFile);
        exit(1);
    }
    if(1!=fwrite(&nOut, 2, 1, hFile)){
        fprintf(stderr, "ERROR : Couldn't write nOut to '%s'.\n", dstFile);
        exit(1);
    }
    
    uint32_t nSynapses=synapses.size();
    if(1!=fwrite(&nSynapses, 4, 1, hFile)){
        fprintf(stderr, "ERROR : Couldn't write nSynapses to '%s'.\n", dstFile);
        exit(1);
    }
    
    if(sizeof(synapse_t)!=8){
        fprintf(stderr, "ERROR : sizeof(synapse_t)!=8\n");
        exit(1);
    }
    
    if(nSynapses!=fwrite(&synapses[0], sizeof(synapse_t), nSynapses, hFile)){
        fprintf(stderr, "Couldn't write %u synapses to '%s'.\n", nSynapses, dstFile);
        exit(1);
    }
    
    if(strcmp(dstFile,"<STDOUT>")){
        fclose(hFile);
    }
}


bool read_input(unsigned n, int8_t *pDest)
{
    unsigned todo=n;
    while(todo > 0){
        auto got=fread(pDest, 1, todo, stdin);
        if(got>0){
            todo -= got;
            pDest += got;
        }
        if(got==0){
            if(todo!=n){
                fprintf(stderr, "Warning : partial input, %u / %u bytes, ferror=%d, feof=%d.\n", (n-todo), n, ferror(stdin), feof(stdin));
            }
            return false;
        }        
    }
    return true;
}

void write_output(unsigned n, const int8_t *pSrc)
{
    auto done=fwrite(pSrc, 1, n, stdout);
    if(done!=n){
        fprintf(stderr, "ERROR : Couldn't write output.\n");
        exit(1);
    }
}
