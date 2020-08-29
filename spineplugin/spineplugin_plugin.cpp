#include "spineplugin_plugin.h"

#include "spineitem.h"

#include <qqml.h>

void SpinepluginPlugin::registerTypes(const char *uri)
{
    // @uri com.mycompany.qmlcomponents
    qmlRegisterType<SpineItem>(uri, 1, 0, "SpineItem");
}

