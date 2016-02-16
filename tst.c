#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define CAT_MAX 15		// максимум категорий
#define KW_MAX 15		// максимум ключевых слов, включая имя файла, в категории
#define VLEN_MAX 96			// максимальная длина строки

typedef struct {
	char *name;
	char *icon;
	char *exec;
	char *categories;
} dtentryT;

// ищет в начале buf подстроку str, если есть, то все после '=' копирует в динамически выделенную память, адрес помещает в savestr
int stringsearch(const char *buf, const char *str, char **savestr);
// проверить наличие директории, если нет, то создать.
int dircheck(const char *dirname);
// разобрать desktop файл, поместить в структуру взятые из него name, icon, exec,  categories
int parsefile(const char *filename, dtentryT *rezult);
// функция для сортировки по именам
int compar(const void *a, const void *b);

int main(int argc, char **argv)
{
	char *appdirs[] = { "/usr/share/applications", "/usr/local/share/applications", "/home/live/.local/share/applications", NULL };
	char *outdir = "/home/live/.jwm/menu";
	char *categorykw[CAT_MAX][KW_MAX] =  {  // сначала имя файла, куда складываем, потом ключевые слова категории
		{ "Desktop", "Desktop", "Screensaver", "Accessibility", NULL },
		{ "System",  "System",   "Monitor",        "Security",      "HardwareSettings", "Core", NULL },
		{"Setup", "Setup", "PackageManager", NULL},
		{"Utility", "Utility", "Viewer", "Development", "Building", "Debugger", "IDE", "Profiling", "Translation", "GUIDesigner", "Archiving","TerminalEmulator","Shell", NULL},
		{"Filesystem","File", NULL},
		{"Graphic","Graphic","Photography","Presentation","Chart", NULL},
		{"Office","Office","Document","WordProcessor","WebDevelopment","TextEditor","Dictionary", NULL},
		{"Calculate","Calculat","Finance","Spreadsheet","ProjectManagement", NULL},
		{"Personal","Personal","Calendar","ContactManagement", NULL},
		{"Network","Network","Dialup","HamRadio","RemoteAccess", NULL},
		{"Internet","Internet","WebBrowser","Email","News","InstantMessaging","Telephony","IRCClient","FileTransfer","P2P", NULL},
		{"Multimedia","Video","Player","Music","Audio","Midi","Mixer","Sequencer","Tuner","TV","DiskBurning", NULL},
		{"Game","Game","Amusement","RolePlaying","Simulation", NULL},
		{ NULL }
																		};  // массив имен файлов категорий и ключевых слов к ним
	FILE *outfiles[CAT_MAX];    // массив файловых указателей категорий
	int catIndex, wordIndex;
	char **appdir;
	DIR *dir;
	struct dirent *entry;
	char buf[80];
	char outfname[64];
	int len, space;
	dtentryT dtentry[300];
	int entryIndex=0;
	int entryIndexMax;
	
	if( ! dircheck(outdir) ) return 1;
	outfname[0]=0;
	strncat(outfname, outdir, sizeof(outfname) - 2 );
	strcat(outfname, "/");
	len = strlen(outfname);
	space = sizeof(outfname) - len - 1;
	for(catIndex=0; catIndex<CAT_MAX; catIndex++){       // по всем категориям
		if( categorykw[catIndex][0] == NULL ) break;   // если пусто, категории кончились
		outfname[len] = 0;			// убрали предыдущее имя файла
		strncat(outfname, categorykw[catIndex][0], space); // записали новое
		if( (outfiles[catIndex] = fopen(outfname, "w")) != 0 )
			fprintf(outfiles[catIndex], "<JWM>\n");
		else
			perror(outfname);
	} // пооткрывали сразу все файлы, потом в конце таким же циклом надо закрыть
	
	for( appdir=appdirs; *appdir != NULL; appdir++){  // по всем каталогам с *.desktop файлами
		dir = opendir(*appdir);
		if(dir){
			buf[0]=0;
			strncat(buf, *appdir, sizeof(buf)-2);
			strcat(buf, "/");
			len = strlen(buf);
			space = sizeof(buf) - len - 1;
			while ((entry = readdir(dir))!=0) {		// перебираем файлы в директории
				if (strstr(entry->d_name,".desktop")==0 ) continue;  // если не *.desktop, этот файл пропускаем
				buf[len]=0;
				strncat(buf,entry->d_name, space);
				entryIndex += parsefile( buf, dtentry + entryIndex);	// разобрали файл, если успешно, заполнять будем следующую ячейку массива
			} /* while readdir */
			closedir(dir);
		} /* if(dir) */
	} // по всем каталогам с *.desktop файлами
	entryIndexMax = entryIndex;

	qsort(dtentry, entryIndexMax, sizeof(dtentryT) , compar );  // рассортировали, то что выбрали из файлов

	for( entryIndex = 0; entryIndex < entryIndexMax; entryIndex++ ){  // Раскидываем сортированные данные по категориям
		for(catIndex=0; catIndex<CAT_MAX; catIndex++){       // по всем категориям
			if( categorykw[catIndex][0] == NULL ) break;   // если пусто, категории кончились
			for( wordIndex=1; wordIndex<KW_MAX; wordIndex++ ){     // по всем ключевым словам категории
				if( categorykw[catIndex][wordIndex] == NULL ) break;         // ключевые слова закончились, выходим из их перебора
				if ( strstr( dtentry[entryIndex].categories, categorykw[catIndex][wordIndex] ) ){
					// если категория нашлась, вывели в соответствующий файл
					fprintf(outfiles[catIndex], "\t<Program label=\"%s\" icon=\"%s\">%s</Program>\n", \
														dtentry[entryIndex].name, dtentry[entryIndex].icon, dtentry[entryIndex].exec);
					break;								// и пошли на следующую категорию
				}
			}
		}
		free(dtentry[entryIndex].name);	// освободили созданные strdup строки
		free(dtentry[entryIndex].icon);
		free(dtentry[entryIndex].exec);
		free(dtentry[entryIndex].categories);
	}

	for(catIndex=0; catIndex<CAT_MAX; catIndex++){       // по всем категориям
		if( categorykw[catIndex][0] == NULL ) break;   // если пусто, категории кончились
		fprintf(outfiles[catIndex], "</JWM>\n");
		fclose(outfiles[catIndex]);
	} // позакрывали все файлы
	return 0;
} /* main() */

// проверить наличие директории, если нет, то создать.
int dircheck(const char *dirname){
	DIR *dir;
	char dncopy[64];
	char *sl;

	if( (dir = opendir(dirname)) != 0 ){
		closedir(dir);						//нормально открылась - закрыть и ничего не делать
		return 1;
	}else{										// не открылась - проверить, есть ли предшествующая, а потом создать
		if( errno != ENOENT ){ perror(dirname); return 0; }	// не открылась не по причине отсутствия
		strncpy(dncopy, dirname, sizeof(dncopy) - 1);
		dncopy[sizeof(dncopy) - 1] = 0;
		if( (sl = strrchr(dncopy, '/')) != NULL ) *sl = 0;	//  подрезали имя по последний слеш
		else return 0;													// нечего подрезать
		if( ! dircheck(dncopy) ) return 0; // предшествующую создать не получилось
		if( mkdir(dirname, 0755) == -1 ){
			perror(dirname);				// не получилось создать
			return 0;
		}
		return 1;
	}
}

// ищет в начале buf подстроку str, если есть, то все после '=' копирует в динамически выделенную память, адрес помещает в savestr
int stringsearch(const char *buf, const char *str, char **savestr){
	char *tp;
	if( strncmp( buf, str, strlen(str) ) == 0 && (tp=strchr(buf, '='))){
		*savestr = strndup(tp+1, 64);
		return 1;
	}
	return 0;
}

// разобрать desktop файл, поместить в структуру взятые из него name, icon, exec, categories
int parsefile(const char *filename, dtentryT *rezult){
	FILE *fp;
	char str[VLEN_MAX];
	char *lf;
	char *namestr;

	fp=fopen(filename,"r");
	if( fp == NULL ){ perror(filename); return 0; }
	memset( rezult, 0, sizeof(dtentryT) );
	while(!feof(fp)) {
		fgets(str,sizeof(str),fp);
		if ( (lf=strchr(str, '\n'))!=0 ) *lf=0;
		if ( stringsearch(str, "Name=", &(rezult->name) ) != 0 ) continue;
		else if ( stringsearch(str, "Name[ru]", &namestr ) != 0 ){  // будет работать если Name[ru] в файле после Name=
					 if( rezult->name ) free( rezult->name );   // если уже было чего-то, освободить память.
					 rezult->name = namestr;
					 continue; }
		else if ( stringsearch(str, "Icon", &(rezult->icon) ) != 0 ) continue;
		else if ( stringsearch(str, "Exec", &(rezult->exec) ) != 0 ) continue;
		else 	  stringsearch(str, "Categories", &(rezult->categories) );
	}  // разобрали *.desktop файл
	fclose(fp);
	if( rezult->name && rezult->exec && rezult->categories  ) return 1;	// необходимые поля заполнены
	else return 0;	// нет нужной информации
}

int compar(const void *a, const void *b){
	dtentryT *la = (dtentryT*)a;
	dtentryT *lb = (dtentryT*)b;

	return strcoll( la->name, lb->name );
}
