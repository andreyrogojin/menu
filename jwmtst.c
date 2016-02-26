#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define CAT_MAX 15		// максимум категорий
#define KW_MAX 17		// максимум ключевых слов, включая название, иконку и размер, в категории
#define FILE_IN_CAT_MAX 50	// максимум пунктов меню в категории
#define DTENTRY_MAX 300		// максимум desktop файлов на входе
#define VLEN_MAX 200			// максимальная длина строки в desktop файле

typedef struct {
	char *name;
	char *icon;
	char *exec;
	char *categories;
} dtentryT;

char *categorykw[CAT_MAX][KW_MAX];		// сначала имя категории, потом иконка, высота меню, потом ключевые слова категории
int catIndex, wordIndex;

// ищет в начале buf подстроку str, если есть, то все после '=' копирует в динамически выделенную память, адрес помещает в savestr
int stringsearch(const char *buf, const char *str, char **savestr);
// разобрать desktop файл, поместить в структуру взятые из него name, icon, exec,  categories
int parsefile(const char *filename, dtentryT *rezult);
// разобрать конфиг файл, результаты разложить в глобальный массив
int parseconfig(const char *filename);
// функция для сортировки по именам
int compar(const void *a, const void *b);

int main(int argc, char **argv)
{
	char *appdirs[] = { "/usr/share/applications", "/usr/local/share/applications", "/home/live/.local/share/applications", NULL };
	char **appdir;
	DIR *dir;
	struct dirent *entry;
	char buf[80];
	int len, space;
	dtentryT dtentry[DTENTRY_MAX];
	int entryIndex=0;
	int entryIndexMax;
	int menuopen = 0;

	strncpy( buf, getenv("HOME"), 25 );
	buf[25]=0;
	strcat( buf, "/.jwm/jwmtst.conf" );
	if( !parseconfig( buf ) ) return 1;	// без конфига делать нечего

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

	printf("<JWM>\n");
	for(catIndex=0; catIndex<CAT_MAX; catIndex++){       // по всем категориям
		if( categorykw[catIndex][0] == NULL ) break;   // если пусто, категории кончились
		if( categorykw[catIndex][0][0] == '_' ) printf("<Separator/>\n"); 	// если имя категории начинается с подчеркивания, это сепаратор
		else
			for( entryIndex = 0; entryIndex < entryIndexMax; entryIndex++ ){  // по всем записям из файлов
				for( wordIndex=3; wordIndex<KW_MAX; wordIndex++ ){     // по всем ключевым словам категории
					if( categorykw[catIndex][wordIndex] == NULL ) break;         // ключевые слова закончились, выходим из их перебора
					if( strstr( dtentry[entryIndex].categories, categorykw[catIndex][wordIndex] ) ){
						if( ! menuopen ){	// открыть меню, если еще не открыто
							printf("<Menu label=\"%s\" icon=\"%s\" height=\"%s\">\n", categorykw[catIndex][0], categorykw[catIndex][1], categorykw[catIndex][2]);
							menuopen = 1;
						}
						printf("\t<Program label=\"%s\" icon=\"%s\">%s</Program>\n", \
							dtentry[entryIndex].name, dtentry[entryIndex].icon, dtentry[entryIndex].exec);
						break;		// вывести запись, и дальше в этой категории слова проверять не надо
					}
				}
			}
		if( menuopen ){ printf("</Menu>\n"); menuopen=0; }	// закрыть меню, если открыто
		for( wordIndex=0; wordIndex<4; wordIndex++ ){
			if( categorykw[catIndex][wordIndex] == NULL ) break;
			free( categorykw[catIndex][wordIndex] );	// эта категория уже выведена, можно освободить память
		}			// не забываем, что под слова, начиная с 3, память выделяли одним блоком
	}
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
	int nameru_found=0;	// флажок

	fp=fopen(filename,"r");
	if( fp == NULL ){ perror(filename); return 0; }
	memset( rezult, 0, sizeof(dtentryT) );		// изначально все пустое
	while(!feof(fp)) {
		fgets(str,sizeof(str),fp);
		if ( (tp=strchr(str, '\n'))!=0 ) *tp=0;
		if ( (str[0] == '[') && (strncmp( str, "[Desktop Entry]", sizeof("[Desktop Entry]")-1 ) != 0) ) break;	// если пошла уже другая секция, дальше не смотрим
		if ( ( (stringsearch(str, "OnlyShowIn", &tmpstr ) != 0) && ( strstr( tmpstr, "Old") == 0) ) \
				|| ( (stringsearch(str, "NoDisplay", &tmpstr ) != 0) && ( strstr( tmpstr, "false") == 0) ) ){	// если файл не для JWM или под NoDisplay, пропустить
			if( rezult->name ) free( rezult->name );
			if( rezult->icon ) free( rezult->icon );		// освободить все найденное
			if( rezult->exec ) free( rezult->exec );
			if( rezult->categories ) free( rezult->categories );
			free(tmpstr);
			return 0;				// и выдать неуспешный статус
		}
		if ( ! nameru_found ) if ( stringsearch(str, "Name=", &(rezult->name) ) != 0 ) continue; 
		if ( stringsearch(str, "Name[ru]", &tmpstr ) != 0 ){
			if( rezult->name ) free( rezult->name );   // если уже было чего-то, освободить память.
			rezult->name = tmpstr;
			nameru_found = 1; // если нашлось Name[ru], то просто Name= искать уже не надо
			continue; }
		if ( stringsearch(str, "Icon", &(rezult->icon) ) != 0 ){ 
			if ( (tp=strrchr(rezult->icon, '/')) != 0 ){
				tmpstr = strndup(tp+1, VLEN_MAX);
				free(rezult->icon);	// если иконка была с путем, вырезать только имя файла, старую строчку освободить
				rezult->icon = tmpstr;	}
			if ( (tp=strrchr(rezult->icon, '.')) != 0 ) *tp=0;	// если с расширением, то его тоже отрезать
			continue; }
		if ( stringsearch(str, "Exec", &(rezult->exec) ) != 0 ) continue;
		stringsearch(str, "Categories", &(rezult->categories) );
	}  // разобрали *.desktop файл
	fclose(fp);
	if( rezult->name && rezult->exec && rezult->categories  ) return 1;	// необходимые поля заполнены
	else return 0;	// нет нужной информации
}

int parseconfig(const char *filename){
	FILE *fp;
	char str[VLEN_MAX], *tp, *tmpstr;
	
	fp=fopen(filename,"r");
	if( fp == NULL ){ perror(filename); return 0; }
	catIndex = -1;
	while( !feof(fp) ){
		if( catIndex == CAT_MAX ){
			fprintf(stderr, "Count \"Name=\" > CAT_MAX in config file\n");
			return 0;		// слишком много категорий в конфиге, не влезли
		}
		fgets(str,sizeof(str), fp);
		if ( (tp=strchr(str, '\n'))!=0 ) *tp=0;
		if ( stringsearch(str, "Name=", &tmpstr ) ){ categorykw[++catIndex][0] = tmpstr; wordIndex=3; continue; }
		if ( stringsearch(str, "Icon=", &categorykw[catIndex][1] ) ){ continue; }
		if ( stringsearch(str, "Height=", &categorykw[catIndex][2] ) ){ continue; }
		if ( stringsearch(str, "Categories=", &tmpstr ) ){
			tp = tmpstr;
			while( (*tp == ' ') || (*tp == '\t') ) tp++;		// пробелы/табуляции пропустить
			if( tp != tmpstr ){
				categorykw[catIndex][wordIndex] = strdup(tp);	// и если пропускали, строчку сделать новую
				free(tmpstr);
			}else categorykw[catIndex][wordIndex] = tmpstr;
			while( (tp=strchr(categorykw[catIndex][wordIndex], ';')) ){
				if( wordIndex == KW_MAX ){
					fprintf(stderr, "Count word in \"Categories=\" > KW_MAX in config file\n");
					return 0;		// слишком много слов в категории, не влезли
				}
				*tp=0;
				tp++;
				while( (*tp == ' ') || (*tp == '\t') ) tp++;		// пробелы/табуляции пропустить
				if( *tp != 0 )	categorykw[catIndex][++wordIndex] = tp;
				else{ categorykw[catIndex][++wordIndex] = NULL; break; }
			}
			continue;
		}
	}
	fclose(fp);
	categorykw[++catIndex][0] = NULL;
	if( catIndex == 0) return 0;	// нет ни одной категории в конфиге
	return 1;
}


int compar(const void *a, const void *b){
	dtentryT *la = (dtentryT*)a;
	dtentryT *lb = (dtentryT*)b;

	return strcoll( la->name, lb->name );
}
