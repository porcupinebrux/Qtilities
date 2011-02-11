/****************************************************************************
**
** Copyright (c) 2009-2011, Jaco Naude
**
** This file is part of Qtilities which is released under the following
** licensing options.
**
** Option 1: Open Source
** Under this license Qtilities is free software: you can
** redistribute it and/or modify it under the terms of the GNU General
** Public License as published by the Free Software Foundation, either
** version 3 of the License, or (at your option) any later version.
**
** Qtilities is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Qtilities. If not, see http://www.gnu.org/licenses/.
**
** Option 2: Commercial
** Alternatively, this library is also released under a commercial license
** that allows the development of closed source proprietary applications
** without restrictions on licensing. For more information on this option,
** please see the project website's licensing page:
** http://www.qtilities.org/licensing.html
**
** If you are unsure which license is appropriate for your use, please
** contact support@qtilities.org.
**
****************************************************************************/

#include "ObserverProjectItemWrapper.h"
#include "ProjectManager.h"

#include <IExportable>
#include <QtilitiesCoreApplication>
#include <Logger>
#include <IFactoryProvider>

#include <QApplication>
#include <QDomNodeList>

struct Qtilities::ProjectManagement::ObserverProjectItemWrapperPrivateData {
    ObserverProjectItemWrapperPrivateData() : observer(0) {}

    QPointer<Observer> observer;
};

using namespace Qtilities::Core::Interfaces;
using namespace Qtilities::Core;

Qtilities::ProjectManagement::ObserverProjectItemWrapper::ObserverProjectItemWrapper(Observer* observer, QObject *parent) :
    QObject(parent)
{
    d = new ObserverProjectItemWrapperPrivateData;
    if (observer)
        setObserverContext(observer);
}

void Qtilities::ProjectManagement::ObserverProjectItemWrapper::setObserverContext(Observer* observer) {
    if (d->observer)
        d->observer->disconnect(this);

    d->observer = observer;
    setObjectName(QString("Observer Project Item: \"%1\"").arg(observer->observerName()));
    IModificationNotifier* mod_iface = qobject_cast<IModificationNotifier*> (observer);
    if (mod_iface)
        connect(mod_iface->objectBase(),SIGNAL(modificationStateChanged(bool)),SLOT(setModificationState(bool)));
}

QString Qtilities::ProjectManagement::ObserverProjectItemWrapper::projectItemName() const {
    if (d->observer)
        return d->observer->observerName() + tr(" Project Item");
    else
        return tr("Uninitialized Observer Project Item Wrapper");
}

bool Qtilities::ProjectManagement::ObserverProjectItemWrapper::newProjectItem() {
    if (!d->observer)
        return false;

    d->observer->deleteAll();
    d->observer->setModificationState(false);
    return true;
}

bool Qtilities::ProjectManagement::ObserverProjectItemWrapper::closeProjectItem() {
    if (!d->observer)
        return false;

    d->observer->startProcessingCycle();
    d->observer->deleteAll();
    d->observer->setModificationState(true);
    d->observer->endProcessingCycle();
    return true;
}

Qtilities::Core::Interfaces::IExportable::ExportModeFlags Qtilities::ProjectManagement::ObserverProjectItemWrapper::supportedFormats() const {
    IExportable::ExportModeFlags flags = 0;
    flags |= IExportable::Binary;
    flags |= IExportable::XML;
    return flags;
}

Qtilities::Core::InstanceFactoryInfo Qtilities::ProjectManagement::ObserverProjectItemWrapper::instanceFactoryInfo() const {
    return InstanceFactoryInfo();
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::ProjectManagement::ObserverProjectItemWrapper::exportBinary(QDataStream& stream, QList<QVariant> params) const {
    Q_UNUSED(params)

    return OBJECT_MANAGER->exportObserverBinary(stream,d->observer,PROJECT_MANAGER->verboseLogging());
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::ProjectManagement::ObserverProjectItemWrapper::importBinary(QDataStream& stream, QList<QPointer<QObject> >& import_list, QList<QVariant> params) {
    Q_UNUSED(import_list)
    Q_UNUSED(params)

    return OBJECT_MANAGER->importObserverBinary(stream,d->observer,PROJECT_MANAGER->verboseLogging());
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::ProjectManagement::ObserverProjectItemWrapper::exportXML(QDomDocument* doc, QDomElement* object_node, QList<QVariant> params) const {
    Q_UNUSED(params)

    if (d->observer) {
        // Add a new node for this observer. We don't want it to add its factory data
        // to the ProjectItem node.
        QDomElement wrapper_data = doc->createElement("ObserverProjectItemWrapper");
        object_node->appendChild(wrapper_data);
        return d->observer->exportXML(doc,&wrapper_data,params);
    } else
        return IExportable::Incomplete;
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::ProjectManagement::ObserverProjectItemWrapper::importXML(QDomDocument* doc, QDomElement* object_node, QList<QPointer<QObject> >& import_list, QList<QVariant> params) {
    Q_UNUSED(params)

    if (d->observer) {
        QDomNodeList childNodes = object_node->childNodes();
        for(int i = 0; i < childNodes.count(); i++)
        {
            QDomNode childNode = childNodes.item(i);
            QDomElement child = childNode.toElement();

            if (child.isNull())
                continue;

            if (child.tagName() == "ObserverProjectItemWrapper") {
                return d->observer->importXML(doc,&child,import_list,params);
            }
        }
    }

    return IExportable::Incomplete;
}

bool Qtilities::ProjectManagement::ObserverProjectItemWrapper::isModified() const {
    if (d->observer)
        return d->observer->isModified();
    else
        return false;
}

void Qtilities::ProjectManagement::ObserverProjectItemWrapper::setModificationState(bool new_state, IModificationNotifier::NotificationTargets notification_targets) {
    if (!d->observer)
        return;

    if (notification_targets & IModificationNotifier::NotifyListeners)
        emit modificationStateChanged(new_state);
    if (notification_targets & IModificationNotifier::NotifySubjects)
        d->observer->setModificationState(new_state,notification_targets);
}
