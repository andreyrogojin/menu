#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define CAT_MAX 15		// максимум категорий
#define KW_MAX 15		// максимум ключевых слов, включая имя файла, в категории
#define VLEN_MAX 96			// максимальная длина строки 

int stringsearch(const char *buf, const char *str, char *savestr);	// ищет в начале buf подстроку str, если есть, то все после '=' копирует в savestr

int main(int argc, char **argv)
{
	char *appdirs[] = { "/usr/share/applications", "/usr/local/share/applications", "/home/live/.local/share/applications", NULL };
	char *categorykw[CAT_MAX][KW_MAX] =  { 
		{ "/home/live/.jwm/menu/Desktop", "Desktop", "Screensaver", "Accessibility", NULL },
		{ "/home/live/.jwm/menu/System",  "System",   "Monitor",        "Security",      "HardwareSettings", "Core", NULL },
		{"/home/live/.jwm/menu/Setup", "Setup", "PackageManager", NULL},
		{"/home/live/.jwm/menu/Utility", "Utility", "Viewer", "Development", "Building", "Debugger", "IDE", "Profiling", "Translation", "GUIDesigner", "Archiving","TerminalEmulator","Shell", NULL},
		{"/home/live/.jwm/menu/Filesystem","File", NULL},
		{"/home/live/.jwm/menu/Graphic","Graphic","Photography","Presentation","Chart", NULL},
		{"/home/live/.jwm/menu/Office","Office","Document","WordProcessor","WebDevelopment","TextEditor","Dictionary", NULL},
		{"/home/live/.jwm/menu/Calculate","Calculat","Finance","Spreadsheet","ProjectManagement", NULL},
		{"/home/live/.jwm/menu/Personal","Personal","Calendar","ContactManagement", NULL},
		{"/home/live/.jwm/menu/Network","Network","Dialup","HamRadio","RemoteAccess", NULL},
		{"/home/live/.jwm/menu/Internet","Internet","WebBrowser","Email","News","InstantMessaging","Telephony","IRCClient","FileTransfer","P2P", NULL},
		{"/home/live/.jwm/menu/Multimedia","Video","Player","Music","Audio","Midi","Mixer","Sequencer","Tuner","TV","DiskBurning", NULL},
		{"/home/live/.jwm/menu/Game","Game","Amusement","RolePlaying","Simulation", NULL},
		{ NULL }
																		};  // массив имен файлов категорий и ключевых слов к ним
	FILE *outfiles[CAT_MAX];    // массив файловых указателей категорий
	int catIndex, wordIndex;
	char **appdir;
	FILE *fp;
	DIR *dir;
	struct dirent *entry;
	char buf[80];
	char *lf;
	int len, space;
	char str[VLEN_MAX];
	char namestr[VLEN_MAX], iconstr[VLEN_MAX], execstr[VLEN_MAX], categorystr[VLEN_MAX];
	
	for(catIndex=0; catIndex<CAT_MAX; catIndex++){       // по всем категориям
		if( categorykw[catIndex][0] == NULL ) break;   // если пусто, категории кончились
		outfiles[catIndex] = fopen(categorykw[catIndex][0], "w");
		fprintf(outfiles[catIndex], "<JWM>\n");
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
				fp=fopen(buf,"r");
				if( fp == NULL ){ fprintf(stderr, "%s not open:", buf); perror(" "); continue; }
				namestr[0]=iconstr[0]=execstr[0]=categorystr[0]=0;  // сбросили, а то в файле не найдем, так останется от предыдущего
				while(!feof(fp)) {                                              
					fgets(str,sizeof(str),fp);                              
					if ( (lf=strchr(str, '\n'))!=0 ) *lf=0;
					if ( stringsearch(str, "Name=", namestr) != 0 ) continue;
					else if ( stringsearch(str, "Name[ru]", namestr) != 0 ) continue;  // будет работать если Name[ru] в файле после Name=
					else if ( stringsearch(str, "Icon", iconstr) != 0 ) continue;
					else if ( stringsearch(str, "Exec", execstr) != 0 ) continue;
					else if ( stringsearch(str, "Categories", categorystr) != 0 ) continue;
				}  // разобрали *.desktop файл
				for(catIndex=0; catIndex<CAT_MAX; catIndex++){       // по всем категориям
					if( categorykw[catIndex][0] == NULL ) break;   // если пусто, категории кончились
					for( wordIndex=1; wordIndex<KW_MAX; wordIndex++ ){     // по всем ключевым словам категории
						if( categorykw[catIndex][wordIndex] == NULL ) break;         // ключевые слова закончились, выходим из их перебора
						if ( strstr(categorystr, categorykw[catIndex][wordIndex]) ){ 
							fprintf(outfiles[catIndex], "\t<Program label=\"%s\" icon=\"%s\">%s</Program>\n", namestr, iconstr, execstr); // если категория нашлась, вывели в соответствующий файл
							break;								// и пошли на следующую категорию
						}
					}
				}
			} /* while readdir */
			closedir(dir);
		} /* if(dir) */
	} // по всем каталогам с *.desktop файлами
	
	for(catIndex=0; catIndex<CAT_MAX; catIndex++){       // по всем категориям
		if( categorykw[catIndex][0] == NULL ) break;   // если пусто, категории кончились
		fprintf(outfiles[catIndex], "</JWM>\n");
		fclose(outfiles[catIndex]);
	} // позакрывали все файлы
	return 0;
}

int stringsearch(const char *buf, const char *str, char *savestr){	// ищет в начале buf подстроку str, если есть, то все после '=' копирует в savestr
	char *tp;
	if( strncmp( buf, str, strlen(str) ) == 0 && (tp=strchr(buf, '='))){ 
		strncpy(savestr, tp+1, VLEN_MAX-1);
		savestr[VLEN_MAX-1]=0;
		return 1;
	}
	return 0;
}
