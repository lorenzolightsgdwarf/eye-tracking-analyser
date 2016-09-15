#ifndef SCENEWALKER_H
#define SCENEWALKER_H

/*Original code from https://github.com/qt/qt3d/blob/5.7/tests/manual/assimp-cpp/main.cpp*/


#include <QObject>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QSceneLoader>
#include <QQmlComponent>
class SceneWalker : public QObject
{
    Q_OBJECT
    Q_PROPERTY( Qt3DRender::QSceneLoader* loader READ loader WRITE setLoader NOTIFY loaderChanged)
public:
    explicit SceneWalker(QObject *parent = 0);
    Qt3DRender::QSceneLoader* loader(){return m_loader;}
    void setLoader(Qt3DRender::QSceneLoader* loader);
    Q_INVOKABLE void startWalk();
signals:
    void loaderChanged();
public slots:

private:
    Qt3DRender::QSceneLoader *m_loader;
    void walkEntity(Qt3DCore::QEntity *e);
    QQmlComponent* m_qqmlcomponent;
};

#endif
