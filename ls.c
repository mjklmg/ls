// HOMEWORK 
// 1. sort with alphabetical order, qsort
// 2. padding/alligment in ouput, so everything is straight without rough edges
//
// sorting:
// a) count the nr of entries
// b) create an array of strings containing names of those etries
// c) sort the array with qsort, print entires
//
// this is so slow tho ....
// but it is not, qsorts requieres u to create comparison function so i can use stat structures and sort them by their st.name and then print those

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>

typedef struct Names {
	char ** items;
	size_t count;
	size_t capacity;
} Names;


void mode_string(mode_t mode, char* str){
    if 		(S_ISSOCK(mode)) 	str[0] = 's';
    else if (S_ISLNK(mode))  	str[0] = 'l';
    else if (S_ISBLK(mode))		str[0] = 'b';
    else if (S_ISDIR(mode))		str[0] = 'd';
    else if (S_ISCHR(mode))		str[0] = 'c';
    else if	(S_ISFIFO(mode))	str[0] = 'p';
    else 						str[0] = '-';

	// we compare mode_t bits with macro checks and construct readable permission string
	str[1] = (mode & S_IRUSR) ? 'r' : '-';
	str[2] = (mode & S_IWUSR) ? 'w' : '-';
	str[3] = (mode & S_IXUSR) ? 'x' : '-';
	str[4] = (mode & S_IRGRP) ? 'r' : '-';
	str[5] = (mode & S_IWGRP) ? 'w' : '-';
	str[6] = (mode & S_IXGRP) ? 'x' : '-';
	str[7] = (mode & S_IROTH) ? 'r' : '-';
	str[8] = (mode & S_IWOTH) ? 'w' : '-';
	str[9] = (mode & S_IXOTH) ? 'x' : '-';
	str[10] = '\0';
}
// lstat abstratction so i can use it in diffrent places easly
int get_stat(const char *dir, const char *name, struct stat *st){
	char fullpath[4096];
	snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);

	if (lstat(fullpath, st) < 0){
		perror(name);
		return -1;

	}
	return 0;
}

void print_long(const char *dir, const char* name, size_t links_width, size_t size_width){

	struct stat st;
	if (get_stat(dir, name, &st)<0){
		return;
	}

	char modes[11];
	mode_string(st.st_mode, modes);

	struct passwd *pw = getpwuid(st.st_uid);
	struct group *gr = getgrgid(st.st_gid);
	const char *user = (pw) ? pw->pw_name : "?";
	const char *group = (gr) ? gr->gr_name : "?";

	char timebuf[64];
	struct tm *tm = localtime(&st.st_mtim.tv_sec);
	strftime(timebuf, sizeof(timebuf), "%H:%M", tm);

	// formated long output with calculated width for links and size
	printf(
			"%s %*lu %s %s %*ld %s %s\n",
			modes,
			(int)links_width,
			(unsigned long)st.st_nlink,
			user,
			group,
			(int)size_width,
			(long)st.st_size,
			timebuf,
			name
			);
}

int comapare_names(const void* x, const void* y){
	const char * name_x = *(const char **)x;
	const char * name_y = *(const char **)y;
	return strcmp(name_x, name_y);
}

int show_all = 0;
int long_format = 0;

size_t links_max_width;
size_t size_max_width;

int main(int argc, char *argv[]){
	int opt;
	while ((opt = getopt(argc, argv, "al")) != -1){
		switch (opt) {
			case 'a':
				show_all = 1;
				break;
			case 'l':
				long_format = 1;
				break;
			default:
				fprintf(stderr, "usage %s [-al] [path]\n", argv[0]);
				return 1;
		}
	}

	const char *path = (optind < argc) ? argv[optind] : "."; //if path provided use it, if not set it too '.'

	DIR *dir= opendir(path);
	if (!dir){
		perror("opendir");
		return 1;
	}

//	printf("%s\n", path);

	struct dirent *entry;

// i should introude array of entry names in this loop so i firstly store the names of the entries and then print them
//
// 1. implement dynamic array
// 2width entry names from readdir() with/out -a sorting
// 3. print the entries from array
//
//
	Names names = {0};

	while ((entry = readdir(dir)) != NULL){
		if (!show_all && entry->d_name[0]=='.') continue;

		if (names.count >= names.capacity) {
			if (names.capacity == 0) names.capacity = 256;
			else names.capacity *= 2;
			char ** tmp = realloc(names.items, names.capacity*sizeof(*names.items));
			if (tmp == NULL){
				perror("realloc");
				return 1;
			}
			names.items = tmp;
		}
		names.items[names.count] = strdup(entry->d_name);

		if (names.items[names.count] == NULL){
			perror("strdup");
			return 1;
		}

		names.count++;
	}

	qsort(names.items, names.count ,sizeof(*names.items), comapare_names);

	// iterating through the entries and calling lstat on them so i can calculate max width for links and size 
	// pretty redundant but whatever, its just an excersize based on tonys tutorial and simnplified version of ls
	// to make it efficient and to avoid systemcalls in a loop complete redesigne is needed
	if (long_format){
		for (size_t i = 0; i < names.count; ++i){
			struct stat st;
			if (get_stat(path,names.items[i], &st)){
				continue;
			}

			char buf[32];

			snprintf(buf, sizeof(buf), "%ld", (long)st.st_size);

			size_t width = strlen(buf);
			if (width>size_max_width){
				size_max_width=width;
			}

			snprintf(buf, sizeof(buf), "%lu", (unsigned long)st.st_nlink);

			width = strlen(buf);
			if (width>links_max_width){
				links_max_width=width;
			}

		}
	}	

	// printing output from array
	for (size_t i = 0; i < names.count; ++i){
		if (long_format){
			print_long(path, names.items[i], links_max_width, size_max_width);
		}
		else {
			printf("%s\n", names.items[i]);
		}
	}

	// cleaning memory
	closedir(dir);
	for (size_t i = 0; i < names.count; ++i){
		free(names.items[i]);
	}
	free(names.items);

	return 0;
}
