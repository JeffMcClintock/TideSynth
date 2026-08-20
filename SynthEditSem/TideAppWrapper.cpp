#include "TideAppWrapper.h"
#include "SynthEditDoc2.h"
#include "TideApp.h"

ISeApp* CreateApp()
{
	auto app = new TideApp();
	app->InitInstance();
	return app;
}
