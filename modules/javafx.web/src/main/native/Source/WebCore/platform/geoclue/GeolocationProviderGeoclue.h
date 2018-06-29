/*
 * Copyright (C) 2012 Igalia S.L.
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef GeolocationProviderGeoclue_h
#define GeolocationProviderGeoclue_h

#if ENABLE(GEOLOCATION)

#include "GeolocationProviderGeoclueClient.h"
#include <wtf/glib/GRefPtr.h>

#include "Geoclue2Interface.h"

namespace WebCore {

class GeolocationProviderGeoclue {
public:
    GeolocationProviderGeoclue(GeolocationProviderGeoclueClient*);
    ~GeolocationProviderGeoclue();

    void startUpdating();
    void stopUpdating();
    void setEnableHighAccuracy(bool);

private:
    static void createGeoclueManagerProxyCallback(GObject*, GAsyncResult*, GeolocationProviderGeoclue*);
    static void getGeoclueClientCallback(GObject*, GAsyncResult*, GeolocationProviderGeoclue*);
    static void createGeoclueClientProxyCallback(GObject*, GAsyncResult*, GeolocationProviderGeoclue*);
    static void startClientCallback(GObject*, GAsyncResult*, GeolocationProviderGeoclue*);
    static void locationUpdatedCallback(GeoclueClient*, const gchar*, const gchar*, GeolocationProviderGeoclue*);
    static void createLocationProxyCallback(GObject*, GAsyncResult*, GeolocationProviderGeoclue*);

    void startGeoclueClient();
    void updateLocation(GeoclueLocation*);

    void errorOccurred(const char*);
    void updateClientRequirements();

    GeolocationProviderGeoclueClient* m_client;

    GRefPtr<GeoclueManager> m_managerProxy;
    GRefPtr<GeoclueClient> m_clientProxy;

    double m_latitude;
    double m_longitude;
    double m_altitude;
    double m_accuracy;
    double m_altitudeAccuracy;
    int m_timestamp;

    bool m_enableHighAccuracy;
    bool m_isUpdating;
};

} // namespace WebCore

#endif // ENABLE(GEOLOCATION)

#endif // GeolocationProviderGeoclue_h
