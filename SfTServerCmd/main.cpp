
#include "MIServer.h"
#include <stdio.h>
#include <winsvc.h>
#include "MiConf.h"


int main(char* argv, int argc) {
	ClaHTTPServerWrapper app;
	app.launch();
	return 0;
}