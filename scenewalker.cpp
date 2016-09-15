#include "scenewalker.h"
#include <QQmlEngine>
#include <QQmlContext>

SceneWalker::SceneWalker(QObject *parent):QObject(parent)
{
    m_loader=Q_NULLPTR;
    m_qqmlcomponent=NULL;

}

void SceneWalker::setLoader(Qt3DRender::QSceneLoader *loader)
{
    m_loader=loader;
    m_qqmlcomponent=new QQmlComponent(qmlEngine(m_loader),m_loader);
    emit loaderChanged();
}

void SceneWalker::startWalk()
{
    if(!m_loader) return;

    if (m_loader->status() != Qt3DRender::QSceneLoader::Loaded)
        return;
    if(!m_qqmlcomponent) return;

    m_qqmlcomponent->loadUrl(QUrl("qrc:/CustomObjPicker.qml"));
    // The QSceneLoader instance is a component of an entity. The loaded scene
    // tree is added under this entity.
    QVector<Qt3DCore::QEntity *> entities = m_loader->entities();

    // Technically there could be multiple entities referencing the scene loader
    // but sharing is discouraged, and in our case there will be one anyhow.
    if (entities.isEmpty())
        return;
    Qt3DCore::QEntity *root = entities[0];
    // Print the tree.
    walkEntity(root);
}

void SceneWalker::walkEntity(Qt3DCore::QEntity *e)
{

    Qt3DCore::QNodeList nodes = e->childrenNodes();
    for (int i = 0; i < nodes.count(); ++i) {
        Qt3DCore::QNode *node = nodes[i];
        Qt3DCore::QEntity *entity = qobject_cast<Qt3DCore::QEntity *>(node);
        if (entity) {
            qDebug()<<entity->objectName();
            QQmlContext* m_qqmlcontext=new QQmlContext(qmlContext(m_loader));
            QObject* _picker=m_qqmlcomponent->create(m_qqmlcontext);
            qDebug()<<m_qqmlcomponent->errorString();
            Qt3DCore::QComponent* picker= qobject_cast<Qt3DCore::QComponent*>(_picker);
            m_qqmlcontext->setContextObject(picker);
            picker->setParent(entity);
            entity->addComponent(picker);
            walkEntity(entity);
        }
    }
}
