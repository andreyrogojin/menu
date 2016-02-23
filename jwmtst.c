#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define CAT_MAX 12		// максимум категорий
#define KW_MAX 17		// максимум ключевых слов, включая название и иконку, в категории
#define FILE_IN_CAT_MAX 50	// максимум пунктов меню в категории
#define DTENTRY_MAX 300		// максимум desktop файлов на входе
#define VLEN_MAX 100			// максимальная длина строки в desktop файле

typedef struct {
	char *name;
	char *icon;
	char *exec;
	char *categories;
} dtentryT;

// ищет в начале buf подстроку str, если есть, то все после '=' копирует в динамически выделенную память, адрес помещает в savestr
int stringsearch(const char *buf, const char *str, char **savestr);
// разобрать desktop файл, поместить в структуру взятые из него name, icon, exec,  categories
int parsefile(const char *filename, dtentryT *rezult);
// функция для сортировки по именам
int compar(const void *a, const void *b);

int main(int argc, char **argv)
{
	char *appdirs[] = { "/usr/share/applications", "/usr/local/share/applications", "/home/live/.local/share/applications", NULL };
	
	char *categorykw[CAT_MAX][KW_MAX] =  {  // сначала имя категории, потом иконка, потом ключевые слова категории
		{"Настройки", "preferences-desktop", "Settings","DesktopSettings","HardwareSettings","Setup","PackageManager","Desktop","Screensaver","Accessibility", NULL },
		{"________", NULL },
		{"Графика", "applications-graphics", "Graphic","Photography","Presentation","Chart", NULL},
		{"Игры", "applications-games", "Game","Amusement","RolePlaying","Simulation", NULL},
		{"Интернет", "applications-internet", "Internet","WebBrowser","Email","News","InstantMessaging","Telephony","IRCClient","FileTransfer","P2P","Network","Dialup","HamRadio","RemoteAccess", NULL},
		{"Инструменты", "applications-accessories", "Utility","Viewer","Profiling","Translation","GUIDesigner","Archiving","TerminalEmulator","Shell","File", NULL},
		{"Мультимедиа", "applications-multimedia", "Video","Player","Music","Audio","Midi","Mixer","Sequencer","Tuner","TV","DiskBurning", NULL},
		{"Офис", "applications-office", "Office","Document","WordProcessor","WebDevelopment","TextEditor","Dictionary","Calculat","Finance","Spreadsheet","ProjectManagement","Personal", "Calendar","ContactManagement", NULL},
		{"Разработка", "applications-development", "Development","Building","Debugger","IDE", NULL},
		{"Система", "applications-system", "System","Monitor","Security","Core", NULL },
		{ NULL }
																		};  // массив категорий и ключевых слов к ним
	int catIndex, wordIndex;
	char **appdir;
	DIR *dir;
	struct dirent *entry;
	char buf[80];
	int len, space;
	dtentryT dtentry[DTENTRY_MAX];
	dtentryT **catentry[CAT_MAX]; // здесь будут указатели на массивы, каждый для своей категории. В массивах указатели на разобранные файлы.
	int catend[CAT_MAX];  // индекс конца каждого из массивов
	int entryIndex=0;
	int entryIndexMax;
	
	for(catIndex=0; catIndex<CAT_MAX; catIndex++){       // по всем категориям
		if( categorykw[catIndex][0] == NULL ) break;   // если пусто, категории кончились
		catentry[catIndex] = calloc(FILE_IN_CAT_MAX, sizeof(dtentryT*) );
		catend[catIndex] = 0;  // пока там пусто
	} // распределили память под массивы категорий
	
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
				if( entryIndex == DTENTRY_MAX ){		// проверка, есть ли куда записывать разобранный файл
					fprintf(stderr, "Too many desktop files\n");
					return 1;  // не помещаются все файлы, аварийный выход
				}
				entryIndex += parsefile( buf, dtentry + entryIndex);	// разобрали файл, если успешно, заполнять будем следующую ячейку массива
			} /* while readdir */
			closedir(dir);
		} /* if(dir) */
	} // по всем каталогам с *.desktop файлами
	entryIndexMax = entryIndex; // запомнили, до какого индекса массив заполнен
	
	qsort(dtentry, entryIndexMax, sizeof(dtentryT) , compar );  // рассортировали, то что выбрали из файлов

	for( entryIndex = 0; entryIndex < entryIndexMax; entryIndex++ ){  // Раскидываем сортированные данные по категориям
		for(catIndex=0; catIndex<CAT_MAX; catIndex++){       // по всем категориям
			if( categorykw[catIndex][0] == NULL ) break;   // если пусто, категории кончились
			if( categorykw[catIndex][0][0] == '_' ) continue; // это сепаратор, пропускаем
			for( wordIndex=2; wordIndex<KW_MAX; wordIndex++ ){     // по всем ключевым словам категории
				if( categorykw[catIndex][wordIndex] == NULL ) break;         // ключевые слова закончились, выходим из их перебора
				if ( strstr( dtentry[entryIndex].categories, categorykw[catIndex][wordIndex] ) ){
					// если категория нашлась, записали адрес ячейки в соотвествующий массив, если там место не кончилось
					if( catend[catIndex] == FILE_IN_CAT_MAX ){
						fprintf(stderr, "Too many entries in category:%s\n", categorykw[catIndex][0]);
						break;  // не помещаются все ссылки в категорию, придется пропустить
					}
					catentry[catIndex][catend[catIndex]] = dtentry + entryIndex;
					catend[catIndex]++;		// пометили, что уровень заполнения увеличился
					break;								// и пошли на следующую категорию
				}
			}
		}
	}
	
	printf("<JWM>\n");
	for(catIndex=0; catIndex<CAT_MAX; catIndex++){       // по всем категориям
		if( categorykw[catIndex][0] == NULL ) break;   // если пусто, категории кончились
		if( categorykw[catIndex][0][0] == '_' ) printf("<Separator/>\n");	// если имя категории начинается с подчеркивания, это сепаратор
		if( catend[catIndex] != 0 ){		// Вывод <Menu>, только если категория не пустая DdShurick
			printf("<Menu label=\"%s\" icon=\"%s\">\n", categorykw[catIndex][0], categorykw[catIndex][1]);
			for(entryIndex=0; entryIndex < catend[catIndex]; entryIndex++){
				printf("\t<Program label=\"%s\" icon=\"%s\">%s</Program>\n", \
						catentry[catIndex][entryIndex]->name, catentry[catIndex][entryIndex]->icon, catentry[catIndex][entryIndex]->exec);
			}
			printf("</Menu>\n");
			
		}
		free(catentry[catIndex]);
	} // вывели категории и освободили память массива категории
	printf("</JWM>\n");
	for( entryIndex = 0; entryIndex < entryIndexMax; entryIndex++ ){
		free(dtentry[entryIndex].name);	// освободили созданные strdup строки
		free(dtentry[entryIndex].icon);
		free(dtentry[entryIndex].exec);
		free(dtentry[entryIndex].categories);
	}
	return 0;
} /* main() */

// ищет в начале buf подстроку str, если есть, то все после '=' копирует в динамически выделенную память, адрес помещает в savestr
int stringsearch(const char *buf, const char *str, char **savestr){
	char *tp;
	if( strncmp( buf, str, strlen(str) ) == 0 && (tp=strchr(buf, '='))){
		*savestr = strndup(tp+1, VLEN_MAX);
		return 1;
	}
	return 0;
}

// разобрать desktop файл, поместить в структуру взятые из него name, icon, exec, categories
int parsefile(const char *filename, dtentryT *rezult){
	FILE *fp;
	char str[VLEN_MAX];
	char *tp;
	char *tmpstr;

	fp=fopen(filename,"r");
	if( fp == NULL ){ perror(filename); return 0; }
	memset( rezult, 0, sizeof(dtentryT) );		// изначально все пустое
	while(!feof(fp)) {
		fgets(str,sizeof(str),fp);
		if ( (tp=strchr(str, '\n'))!=0 ) *tp=0;
		if ( ( (stringsearch(str, "OnlyShowIn", &tmpstr ) != 0) && ( strstr( tmpstr, "Old") == 0) ) \
				|| ( (stringsearch(str, "NoDisplay", &tmpstr ) != 0) && ( strstr( tmpstr, "false") == 0) ) ){	// если файл не для JWM или под NoDisplay, пропустить
			if( rezult->name ) free( rezult->name );
			if( rezult->icon ) free( rezult->icon );		// освободить все найденное
			if( rezult->exec ) free( rezult->exec );
			if( rezult->categories ) free( rezult->categories );
			free(tmpstr);
			return 0;				// и выдать неуспешный статус
		}
		else if ( stringsearch(str, "Name=", &(rezult->name) ) != 0 ) continue;
		else if ( stringsearch(str, "Name[ru]", &tmpstr ) != 0 ){  // будет работать если Name[ru] в файле после Name=
					 if( rezult->name ) free( rezult->name );   // если уже было чего-то, освободить память.
					 rezult->name = tmpstr;
					 continue; }
		else if ( stringsearch(str, "Icon", &(rezult->icon) ) != 0 ){ 
					if ( (tp=strrchr(rezult->icon, '/')) != 0 ){
						tmpstr = strndup(tp+1, VLEN_MAX);
						free(rezult->icon);	// если иконка была с путем, вырезать только имя файла, старую строчку освободить
						rezult->icon = tmpstr;
					}
					if ( (tp=strrchr(rezult->icon, '.')) != 0 ) *tp=0;	// если с расширением, то его тоже отрезать
					continue; }
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
