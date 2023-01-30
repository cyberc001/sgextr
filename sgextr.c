#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MPK_ERR_EOF		-1
#define MPK_ERR_SIG		-2
#define MPK_ERR_MEM		-3
#define MPK_ERR_WRITE	-4

typedef struct{
	char sig[4];
	uint32_t unknown;
	uint32_t fent_cnt;	// count of file entries
} mpk_header;

typedef struct{
	uint32_t fnum;		// file number
	uint64_t foff;		// file offset
	uint64_t fsize;		// file size
	uint64_t fsize2;	// 2nd file size (for redundancy?)
	char fname[224];
} mpk_file_entry;

int mpk_read_header(FILE* fd, mpk_header* hdr_out);
int mpk_start_reading_file_entries(FILE* fd);
int mpk_read_file_entry(FILE* fd, mpk_file_entry* fent_out);
int mpk_extract_file(FILE* fd, FILE* fout, mpk_file_entry* fent);

int main(int argc, char** argv)
{
	if(argc < 2){
		fprintf(stderr, "Usage: %s <MPK archive file name> [file to extract 1] [file to extract 2] ...\n"
				"Specify no files to list files in a MPK archive.\n"
				"Extract all files by specifying 'all' as first file name.\n",
				argv[0]);
		return -1;
	}

	FILE* fd = fopen(argv[1], "rb");
	if(!fd){
		fprintf(stderr, "Couldn't open file \"%s\".\n", argv[1]);
		return -2;
	}

	int err;
	mpk_header hdr;
	if(err = mpk_read_header(fd, &hdr)){
		fprintf(stderr, "Error reading MPK header: %d\n", err);
		return err;
	}

	if(argc == 2){ // just list files
		printf("MPK archive \"%s\" contains %u files:\n", argv[1], hdr.fent_cnt);
		if(err = mpk_start_reading_file_entries(fd)){
			fprintf(stderr, "Error reading MPK file entries: %d\n", err);
			return err;
		}
		mpk_file_entry fent;
		for(uint32_t i = 0; i < hdr.fent_cnt; ++i){
			if(err = mpk_read_file_entry(fd, &fent)){
				fprintf(stderr, "Error reading MPK file entries: %d\n", err);
				return err;
			}
			printf("%s\n", fent.fname);
		}
	}
	else{
		if(err = mpk_start_reading_file_entries(fd)){
			fprintf(stderr, "Error reading MPK file entries: %d\n", err);
			return err;
		}
		mpk_file_entry fent;

		char* found_names = calloc(argc - 2, 1);
		for(uint32_t i = 0; i < hdr.fent_cnt; ++i){
			if(err = mpk_read_file_entry(fd, &fent)){
				fprintf(stderr, "Error reading MPK file entries: %d\n", err);
				return err;
			}
			if(strcmp(argv[2], "all")){
				int j; for(j = 2; j < argc; ++j){ // filter by specified names
					if(!strcmp(argv[j], fent.fname)){ // found a file, extract it
						FILE* fout = fopen(fent.fname, "wb");
						if(!fout)
							fprintf(stderr, "Couldn't open file \"%s\" for writing\n", fent.fname);
						else{
							long off = ftell(fd); // save current file entry offset
							if(err = mpk_extract_file(fd, fout, &fent))
								fprintf(stderr, "Couldn't extract file \"%s\": error %d\n", fent.fname, err);
							fclose(fout);
							fseek(fd, off, SEEK_SET);
						}
						found_names[j - 2] = 1;
						break;
					}
				}
			}
			else{ // extract all files
				FILE* fout = fopen(fent.fname, "wb");
				if(!fout)
					fprintf(stderr, "Couldn't open file \"%s\" for writing\n", fent.fname);
				else{
					long off = ftell(fd); // save current file entry offset
					if(err = mpk_extract_file(fd, fout, &fent))
						fprintf(stderr, "Couldn't extract file \"%s\": error %d\n", fent.fname, err);
					fclose(fout);
					fseek(fd, off, SEEK_SET);
				}
			}
		}
		// Announce all files that were not found
		if(strcmp(argv[2], "all"))
			for(int i = 0; i < argc - 2; ++i)
				if(!found_names[i])
					fprintf(stderr, "Couldn't find file \"%s\"\n", argv[i + 2]);
		free(found_names);
	}
	fclose(fd);
}

int mpk_read_header(FILE* fd, mpk_header* hdr_out)
{
	fseek(fd, 0, SEEK_SET);
	if(!fread(hdr_out, sizeof(mpk_header), 1, fd))
		return MPK_ERR_EOF;

	static const char mpk_sig[4] = {'M', 'P', 'K', '\0'};
	for(size_t i = 0; i < 4; ++i)
		if(hdr_out->sig[i] != mpk_sig[i])
			return MPK_ERR_SIG;
	return 0;
}

int mpk_start_reading_file_entries(FILE* fd)
{
	if(fseek(fd, 0x40, SEEK_SET) == -1)
		return MPK_ERR_EOF;
	return 0;
}
int mpk_read_file_entry(FILE* fd, mpk_file_entry* fent_out)
{
	if(!fread(fent_out, sizeof(mpk_file_entry), 1, fd))
		return MPK_ERR_EOF;
	return 0;
}

int mpk_extract_file(FILE* fd, FILE* fout, mpk_file_entry* fent)
{
	if(fseek(fd, fent->foff, SEEK_SET) == -1)
		return MPK_ERR_EOF;
	void* buf = malloc(fent->fsize);
	if(!buf)
		return MPK_ERR_MEM;
	if(fread(buf, 1, fent->fsize, fd) != fent->fsize){
		free(buf);
		return MPK_ERR_EOF;
	}
	if(fwrite(buf, 1, fent->fsize, fout) != fent->fsize){
		free(buf);
		return MPK_ERR_WRITE;
	}
	return 0;
}
