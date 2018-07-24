enum {
	DUMP_SELF,
	DUMP_ELF
};

Menu *dumpGenerateModuleMenu(Menu *p_menu, int index);
Menu *dumpGenerateMenu();
void dumpStart();
int dumpParseFolder(Menu *menu, const char *path);