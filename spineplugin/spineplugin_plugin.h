#ifndef SPINEPLUGIN_PLUGIN_H
#define SPINEPLUGIN_PLUGIN_H

#include <QQmlExtensionPlugin>

class SpinepluginPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
};

#endif // SPINEPLUGIN_PLUGIN_H
