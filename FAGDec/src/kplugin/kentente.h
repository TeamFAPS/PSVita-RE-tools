enum Status {
	ENTENTE_NONE,
	ENTENTE_UPDATE,
	ENTENTE_DONE,
	ENTENTE_DONESEG
};
typedef struct ententeParams {
	char *path;
	char *outpath;
	char *rifpath;
	int path_id;
	int usecdram;
	int self_auth;
	int self_type;
} ententeParams;

int kuEntenteDecryptSelf(ententeParams *u_param) ;
int	kuEntenteStatus();
void kuEntenteGetLogs(char *dest);