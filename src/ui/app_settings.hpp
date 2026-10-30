#pragma once
// Settings exposed to QML. Station fields live in the existing station file
// (platform/settings, shared with the CLI and the legacy editor); app options
// in an INI file; account credentials in a separate owner-only (0600) file so
// they never sit next to shareable settings. All files are under the per-user
// config directory unless an explicit station path is given.
#include "../platform/settings.hpp"
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <memory>

namespace hamclock {

class CredentialStore;

class AppSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString callsign READ callsign NOTIFY stationChanged)
    Q_PROPERTY(double deLatitude READ deLatitude NOTIFY stationChanged)
    Q_PROPERTY(double deLongitude READ deLongitude NOTIFY stationChanged)
    Q_PROPERTY(bool hasDx READ hasDx NOTIFY stationChanged)
    Q_PROPERTY(double dxLatitude READ dxLatitude NOTIFY stationChanged)
    Q_PROPERTY(double dxLongitude READ dxLongitude NOTIFY stationChanged)
    Q_PROPERTY(QString deZone READ deZone NOTIFY stationChanged)
    Q_PROPERTY(QString appearance READ appearance WRITE setAppearance NOTIFY appearanceChanged)
    Q_PROPERTY(bool configured READ configured NOTIFY stationChanged)
    // Why the station file could not be read at start ("" when it was fine or absent).
    Q_PROPERTY(QString loadError READ loadError NOTIFY stationChanged)
    Q_PROPERTY(QString clusterHost READ clusterHost NOTIFY optionsChanged)
    Q_PROPERTY(int clusterPort READ clusterPort NOTIFY optionsChanged)
    Q_PROPERTY(int clusterSsid READ clusterSsid NOTIFY optionsChanged)
    Q_PROPERTY(QString clusterCommandsText READ clusterCommandsText NOTIFY optionsChanged)
    Q_PROPERTY(QString watchListText READ watchListText NOTIFY optionsChanged)
    Q_PROPERTY(QString adifPath READ adifPath NOTIFY optionsChanged)
    Q_PROPERTY(QString otaProgram READ otaProgram NOTIFY optionsChanged)
    Q_PROPERTY(QString otaBand READ otaBand NOTIFY optionsChanged)
    Q_PROPERTY(QString otaMode READ otaMode NOTIFY optionsChanged)
    Q_PROPERTY(int otaMaxAgeMin READ otaMaxAgeMin NOTIFY optionsChanged)
    Q_PROPERTY(QString otaSort READ otaSort NOTIFY optionsChanged)
    Q_PROPERTY(QString aprsHost READ aprsHost NOTIFY optionsChanged)
    Q_PROPERTY(int aprsPort READ aprsPort NOTIFY optionsChanged)
    Q_PROPERTY(QString meshHost READ meshHost NOTIFY optionsChanged)
    Q_PROPERTY(int meshPort READ meshPort NOTIFY optionsChanged)
    // Station hardware on this computer or the LAN (not on iOS): rigctld, rotctld, PiAware, gpsd / serial GPS.
    Q_PROPERTY(QString radioHost READ radioHost NOTIFY optionsChanged)
    Q_PROPERTY(int radioPort READ radioPort NOTIFY optionsChanged)
    Q_PROPERTY(QString rotatorHost READ rotatorHost NOTIFY optionsChanged)
    Q_PROPERTY(int rotatorPort READ rotatorPort NOTIFY optionsChanged)
    Q_PROPERTY(QString adsbHost READ adsbHost NOTIFY optionsChanged)
    Q_PROPERTY(int adsbPort READ adsbPort NOTIFY optionsChanged)
    Q_PROPERTY(QString gpsHost READ gpsHost NOTIFY optionsChanged)
    Q_PROPERTY(int gpsPort READ gpsPort NOTIFY optionsChanged)
    Q_PROPERTY(QString gpsDevice READ gpsDevice NOTIFY optionsChanged)
    Q_PROPERTY(int gpsBaud READ gpsBaud NOTIFY optionsChanged)
    Q_PROPERTY(int pskMaxKm READ pskMaxKm NOTIFY optionsChanged)
    Q_PROPERTY(QString pskMatch READ pskMatch NOTIFY optionsChanged)
    Q_PROPERTY(bool pskWspr READ pskWspr NOTIFY optionsChanged)
    Q_PROPERTY(bool pskRbn READ pskRbn NOTIFY optionsChanged)
    Q_PROPERTY(int aprsRadiusKm READ aprsRadiusKm NOTIFY optionsChanged)
    Q_PROPERTY(int udpSpotPort READ udpSpotPort NOTIFY optionsChanged)
    Q_PROPERTY(bool imperialUnits READ imperialUnits NOTIFY optionsChanged)
    Q_PROPERTY(int dateOrder READ dateOrder NOTIFY optionsChanged)
    Q_PROPERTY(bool clock12h READ clock12h NOTIFY optionsChanged)
    Q_PROPERTY(int bannerStyle READ bannerStyle NOTIFY optionsChanged)
    Q_PROPERTY(int bannerBackground READ bannerBackground NOTIFY optionsChanged)
    Q_PROPERTY(QString bannerTitle READ bannerTitle NOTIFY optionsChanged)
    Q_PROPERTY(QString dxZoneOverride READ dxZoneOverride NOTIFY optionsChanged)
    Q_PROPERTY(bool weekStartsSunday READ weekStartsSunday NOTIFY optionsChanged)
    Q_PROPERTY(int overscanPercent READ overscanPercent NOTIFY optionsChanged)
    Q_PROPERTY(int textScalePercent READ textScalePercent NOTIFY optionsChanged)
    Q_PROPERTY(int accentStyle READ accentStyle NOTIFY optionsChanged)
    Q_PROPERTY(QString mapSize READ mapSize NOTIFY optionsChanged)
    Q_PROPERTY(QString orientationLock READ orientationLock NOTIFY optionsChanged)
    Q_PROPERTY(int bigClockFace READ bigClockFace NOTIFY optionsChanged)
    Q_PROPERTY(int blankIdleMinutes READ blankIdleMinutes NOTIFY optionsChanged)
    Q_PROPERTY(int blankOffHour READ blankOffHour NOTIFY optionsChanged)
    Q_PROPERTY(int blankOnHour READ blankOnHour NOTIFY optionsChanged)
    Q_PROPERTY(int blankDays READ blankDays NOTIFY optionsChanged)
    Q_PROPERTY(bool udpSpotLan READ udpSpotLan NOTIFY optionsChanged)
    Q_PROPERTY(QString udpSpotGroup READ udpSpotGroup NOTIFY optionsChanged)
    Q_PROPERTY(bool udpSpotByMe READ udpSpotByMe NOTIFY optionsChanged)
    Q_PROPERTY(bool udpSetsDx READ udpSetsDx NOTIFY optionsChanged)
    Q_PROPERTY(QString lookupService READ lookupService NOTIFY accountsChanged)
    Q_PROPERTY(QString lookupUser READ lookupUser NOTIFY accountsChanged)
    // Widget id shown in each pane slot of the dashboard ("none" = empty slot).
    Q_PROPERTY(QStringList panes READ panes NOTIFY panesChanged)
    // Seconds between automatic widget changes per slot (0 = off; 5, 10, 30 or 60).
    Q_PROPERTY(QList<int> paneRotations READ paneRotations NOTIFY panesChanged)
    // Demo mode: every pane steps through all widgets every kDemoSeconds. Session only, never saved.
    Q_PROPERTY(bool demoMode READ demoMode WRITE setDemoMode NOTIFY demoModeChanged)
    // Widgets a slot's rotation cycles through, one list per slot (empty list = every widget).
    Q_PROPERTY(QVariantList paneRotationSets READ paneRotationSets NOTIFY panesChanged)
    Q_PROPERTY(bool hasLookupPassword READ hasLookupPassword NOTIFY accountsChanged)
    Q_PROPERTY(QString hamAlertUser READ hamAlertUser NOTIFY accountsChanged)
    Q_PROPERTY(bool hasHamAlertPassword READ hasHamAlertPassword NOTIFY accountsChanged)
    Q_PROPERTY(bool hasHamsatKey READ hasHamsatKey NOTIFY accountsChanged)
    // Settings > Accounts: one sign-in per service (QRZ.com and HamQTH can both be signed in).
    Q_PROPERTY(QString qrzUser READ qrzUser NOTIFY accountsChanged)
    Q_PROPERTY(QString hamqthUser READ hamqthUser NOTIFY accountsChanged)
public:
    // station_path empty = <config>/station.conf. config_dir empty = per-user default.
    explicit AppSettings(QString station_path = {}, QString config_dir = {}, QObject* parent = nullptr);
    ~AppSettings() override;
    // Keeps passwords and API keys in `store` (the Keychain on Apple devices)
    // instead of credentials.ini, moving any already in the file there. Without
    // one they stay in the owner-only credentials.ini (the Pi).
    void setSecretStore(std::unique_ptr<CredentialStore> store);

    QString callsign() const { return QString::fromStdString(station_.callsign); }
    double deLatitude() const { return station_.de.latitude; }
    double deLongitude() const { return station_.de.longitude; }
    bool hasDx() const { return configured_ && has_dx(station_); }  // false when DX is unset (DX == DE)
    double dxLatitude() const { return station_.dx.latitude; }
    double dxLongitude() const { return station_.dx.longitude; }
    QString deZone() const { return QString::fromStdString(station_.de_zone); }
    QString appearance() const { return QString::fromStdString(station_.appearance); }
    bool configured() const { return configured_; }
    const StationSettings& station() const { return station_; }
    QString clusterHost() const { return cluster_host_; }
    int clusterPort() const { return cluster_port_; }
    int clusterSsid() const { return cluster_ssid_; }                  // 0 = none, else 1-99
    QStringList clusterCommands() const { return cluster_commands_; } // sent after login
    QString clusterCommandsText() const { return cluster_commands_.join('\n'); }
    QStringList watchList() const { return watch_list_; }              // uppercase calls / prefixes with *
    QString watchListText() const { return watch_list_.join(' '); }
    int udpSpotPort() const { return udp_port_; }                      // 0 = UDP spot input off
    int dateOrder() const { return date_order_; }                      // 0 D M Y, 1 M D Y, 2 ISO
    bool clock12h() const { return clock_12h_; }
    int bannerStyle() const { return banner_style_; }
    int bannerBackground() const { return banner_bg_; }
    QString bannerTitle() const { return banner_title_; }
    QString dxZoneOverride() const { return dx_zone_override_; }
    bool weekStartsSunday() const { return week_sunday_; }
    int overscanPercent() const { return overscan_; }
    int textScalePercent() const { return text_scale_; }
    // 0 blue (default), 1 green, 2 amber, 3 red, 4 purple.
    int accentStyle() const { return accent_style_; }
    // "large" (default), "medium" or "small": how much height the map keeps; a smaller map leaves room for a second row of panes.
    QString mapSize() const { return map_size_; }
    // "auto" (follow the device), "portrait" or "landscape". Applied on iPhone/iPad only.
    QString orientationLock() const { return orientation_lock_; }
    int bigClockFace() const { return big_clock_face_; }
    int blankIdleMinutes() const { return blank_idle_; }
    int blankOffHour() const { return blank_off_; }
    int blankOnHour() const { return blank_on_; }
    int blankDays() const { return blank_days_; }                      // bit 0 = Monday .. bit 6 = Sunday
    bool imperialUnits() const { return imperial_; }                   // distances in miles
    bool udpSetsDx() const { return udp_sets_dx_; }                    // UDP spots may set a session-only DX target
    bool dxOverrideActive() const { return dx_override_; }
    bool udpSpotByMe() const { return udp_by_me_; }                    // true = only spots from my own call/programs
    QString udpSpotGroup() const { return udp_group_; }               // IPv4 multicast group, empty = unicast
    // false = 127.0.0.1 only. Always true on iOS: WSJT-X never runs on the phone itself.
#ifdef Q_OS_IOS
    bool udpSpotLan() const { return true; }
#else
    bool udpSpotLan() const { return udp_lan_; }
#endif
    // On-the-air display filter; empty / 0 = no restriction.
    QString otaProgram() const { return ota_program_; }  // "POTA", "SOTA", "WWFF" or empty
    QString otaBand() const { return ota_band_; }        // e.g. "20m"
    QString otaMode() const { return ota_mode_; }        // e.g. "CW"
    int otaMaxAgeMin() const { return ota_max_age_min_; }
    QString otaSort() const { return ota_sort_; }
    // APRS-IS server (receive-only) and search radius around DE.
    QString aprsHost() const { return aprs_host_; }
    int aprsPort() const { return aprs_port_; }
    // Local Meshtastic node (TCP API, receive-only). Empty host = off (opt-in, nothing is contacted).
    QString meshHost() const { return mesh_host_; }
    int meshPort() const { return mesh_port_; }
    // Station hardware endpoints; hosts default to this computer (127.0.0.1).
    QString radioHost() const { return radio_host_; }
    int radioPort() const { return radio_port_; }
    QString rotatorHost() const { return rotator_host_; }
    int rotatorPort() const { return rotator_port_; }
    QString adsbHost() const { return adsb_host_; }
    int adsbPort() const { return adsb_port_; }
    QString gpsHost() const { return gps_host_; }
    int gpsPort() const { return gps_port_; }
    QString gpsDevice() const { return gps_device_; }   // serial NMEA receiver; empty = use gpsd
    int gpsBaud() const { return gps_baud_; }
    int pskMaxKm() const { return psk_max_km_; }
    QString pskMatch() const { return psk_match_; }
    bool pskRbn() const { return psk_rbn_; }  // also listen to the Reverse Beacon Network telnet feed (off by default)
    bool pskWspr() const { return psk_wspr_; }  // also query wspr.live (off by default)
    int aprsRadiusKm() const { return aprs_radius_km_; }
    QString adifPath() const { return adif_path_; }                    // local .adi log, empty = none
    // The lookup service callsign lookups use: QRZ.com when signed in there, else HamQTH, else "none".
    QString lookupService() const;
    QString lookupUser() const { return lookupUserFor(lookupService()); }
    bool hasLookupPassword() const { return !lookupPassword().isEmpty(); }
    // C++ only (never a QML property): for widgets that log in.
    QString lookupPassword() const { return lookupPasswordFor(lookupService()); }
    QString lookupUserFor(const QString& service) const { return account_user_.value(service); }
    QString lookupPasswordFor(const QString& service) const { return account_secret_.value(service); } // C++ only
    QString qrzUser() const { return account_user_.value("qrz"); }
    QString hamqthUser() const { return account_user_.value("hamqth"); }
    QString hamAlertUser() const { return account_user_.value("hamalert"); }
    bool hasHamAlertPassword() const { return !hamAlertPassword().isEmpty(); }
    QString hamAlertPassword() const { return account_secret_.value("hamalert"); } // C++ only, like lookupPassword()
    bool hasHamsatKey() const { return !hamsatKey().isEmpty(); }
    QString hamsatKey() const { return account_secret_.value("hamsat"); } // C++ only (never a QML property)
    QString stationPath() const { return station_path_; }
    QString optionsPath() const { return config_dir_ + "/options.ini"; }
    QString loadError() const { return load_error_; }

    static constexpr int kPaneSlots = 12;
    static constexpr int kMaxRotationSet = 64;
    QStringList panes() const;
    // Returns an empty string or a user-facing error.
    Q_INVOKABLE QString setPane(int slot, const QString& widget_id);
    // Shows a widget in a pane for this run only (the --pane screenshot option); nothing is saved.
    QString showPane(int slot, const QString& widget_id);
    Q_INVOKABLE void resetPanes();
    QList<int> paneRotations() const;
    static constexpr int kDemoSeconds = 10;
    bool demoMode() const { return demo_mode_; }
    void setDemoMode(bool on);
    static bool validRotation(int seconds) { return seconds == 0 || seconds == 5 || seconds == 10 || seconds == 30 || seconds == 60; }
    Q_INVOKABLE QString setPaneRotation(int slot, int seconds);
    QVariantList paneRotationSets() const;
    // Add the widget to the slot's rotation set, or remove it if present. An empty set means all widgets.
    Q_INVOKABLE QString togglePaneRotationMember(int slot, const QString& widget_id);
    Q_INVOKABLE QString clearPaneRotationSet(int slot);

    void setAppearance(const QString& appearance);
    // Validate and save; returns an empty string or a user-facing error.
    Q_INVOKABLE QString saveStation(const QString& callsign, double de_lat, double de_lon,
                                    double dx_lat, double dx_lon, const QString& de_zone);
    Q_INVOKABLE QString saveCluster(const QString& host, int port);
    // Public nodes offered in Settings ([{label, host, port, note}]); never connected to unasked.
    Q_INVOKABLE QVariantList clusterSuggestions() const;
    // SSID 0-99 (0 = none); commands one per line, filtered to receive-only commands.
    Q_INVOKABLE QString saveClusterLogin(int ssid, const QString& commands);
    Q_INVOKABLE QString saveWatchList(const QString& entries); // space/comma separated
    Q_INVOKABLE QString saveAdifPath(const QString& path);
    // program: POTA, SOTA, WWFF or empty; band like 20m; mode letters/digits; age 0-1440 min (0 = any).
    Q_INVOKABLE QString saveOtaSort(const QString& sort);
    Q_INVOKABLE QString saveOtaFilter(const QString& program, const QString& band, const QString& mode, int max_age_min);
    // host letters/digits/dots/hyphens (empty = rotate.aprs2.net); port 1-65535; radius 1-500 km.
    Q_INVOKABLE QString saveAprs(const QString& host, int port, int radius_km);
    // host letters/digits/dots/hyphens, empty = off; port 1-65535 (node default 4403).
    Q_INVOKABLE QString saveMesh(const QString& host, int port);
    // Station hardware: host letters/digits/dots/hyphens/colons (empty = this computer); port 1-65535.
    Q_INVOKABLE QString saveRadio(const QString& host, int port);
    Q_INVOKABLE QString saveRotator(const QString& host, int port);
    Q_INVOKABLE QString saveAdsb(const QString& host, int port);
    // device: empty (use gpsd) or a /dev/tty* or /dev/serial/* path; baud 4800-115200 (usual NMEA rates).
    Q_INVOKABLE QString saveGps(const QString& host, int port, const QString& device, int baud);
    // PSKReporter display limit: only receivers within max_km of DE (0-20000, 0 = any).
    Q_INVOKABLE QString savePskMaxKm(int max_km);
    // Receiver call/grid prefixes to keep ("VK, JA, PM95"); empty = all. Max 8 tokens.
    Q_INVOKABLE QString savePskMatch(const QString& filter);
    Q_INVOKABLE QString savePskWspr(bool on);
    Q_INVOKABLE QString savePskRbn(bool on);
    Q_INVOKABLE QString saveImperialUnits(bool imperial);
    Q_INVOKABLE QString saveClockFormat(int date_order, bool h12);
    Q_INVOKABLE QString saveWeekStart(bool sunday);
    Q_INVOKABLE QString saveBannerStyle(int style);
    Q_INVOKABLE QString saveBannerBackground(int style);
    Q_INVOKABLE QString saveBannerTitle(const QString& title);
    /// Saved configurations: named copies of options.ini (never station or credentials). Restoring
    /// reloads AppSettings and the pane slots at once, and blanking follows optionsChanged;
    /// alarms read options.ini at start, so those need a restart.
    Q_PROPERTY(QStringList configNames READ configNames NOTIFY configsChanged)
    QStringList configNames() const;
    Q_INVOKABLE QString saveConfig(const QString& name);
    Q_INVOKABLE QString restoreConfig(const QString& name);
    Q_INVOKABLE QString deleteConfig(const QString& name);
    Q_INVOKABLE QString renameConfig(const QString& from, const QString& to);
    /// Manual DX time zone (IANA id); empty = automatic from the DX coordinates.
    Q_INVOKABLE QString saveDxZone(const QString& zone_id);
    // TV overscan inset: 0..10 % of width/height kept free on every edge.
    Q_INVOKABLE QString saveOverscan(int percent);
    // Large-type option for TVs and the 7-inch screen: 100, 115, 130 or 150 %.
    Q_INVOKABLE QString saveTextScale(int percent);
    Q_INVOKABLE QString saveAccentStyle(int style);
    Q_INVOKABLE QString saveMapSize(const QString& size);
    Q_INVOKABLE QString saveOrientationLock(const QString& lock);
    // Pane columns and rows that fit the pane area at the current map size, and the area's height: {columns, rows, height}.
    Q_INVOKABLE QVariantMap paneGrid(double width, double height, double gap, double min_pane_width, double min_pane_height) const;
    /// Big Clock page face: 0 digital, 1 analog dial above the digital lines.
    Q_INVOKABLE QString saveBigClockFace(int face);
    // Screen blanking: idle minutes (0 = never, max 240) and a daily window off_hour..on_hour
    // (local, 0..23; -1 and -1 = none). Software overlay only.
    Q_INVOKABLE QString saveBlanking(int idle_minutes, int off_hour, int on_hour);
    Q_INVOKABLE QString saveBlankDays(int days);
    Q_INVOKABLE QString saveUdpSpots(int port, bool lan);
    Q_INVOKABLE QString saveUdpGroup(const QString& group);
    Q_INVOKABLE QString saveUdpScope(bool by_me);
    Q_INVOKABLE QString saveUdpSetsDx(bool on);
    // Session-only DX target (from UDP spots): never written to the station file; a later explicit saveStation
    // with other DX coordinates replaces it. Invalid coordinates are refused.
    bool setDxOverride(double lat, double lon);
    void clearDxOverride();
    // Per-source map on/off for spot layers (keys dx psk ota hamalert aprs mesh adif); default on.
    Q_INVOKABLE bool mapSourceVisible(const QString& key) const { return !map_hidden_.contains(key); }
    Q_INVOKABLE QString saveMapSourceVisible(const QString& key, bool visible);
    // Spot label style per source: -1 = the layer's own, else 0 None, 1 Prefix, 2 Call, 3 Dot.
    Q_INVOKABLE int mapSourceLabel(const QString& key) const { return map_label_.value(key, -1); }
    Q_INVOKABLE QString saveMapSourceLabel(const QString& key, int mode);
    // Minimum distance between spot labels in px (0 = each layer's own).
    Q_INVOKABLE int mapMinLabelPx() const { return map_min_label_px_; }
    Q_INVOKABLE QString saveMapMinLabelPx(int px);
    // Settings > Accounts. service: "qrz", "hamqth", "hamalert" or "hamsat" (key in `secret`, no user).
    // An empty secret keeps the stored one for the same user. Returns "" or a message for the user.
    Q_INVOKABLE QString signIn(const QString& service, const QString& user, const QString& secret);
    Q_INVOKABLE QString signOut(const QString& service);
    Q_INVOKABLE QString accountUser(const QString& service) const { return account_user_.value(service); }
    // User and password (or the hams.at key) stored for the service.
    Q_INVOKABLE bool accountSignedIn(const QString& service) const;
    // Older single-account form: signs in to `service` and out of the other lookup service
    // ("none" signs out of both). Empty password keeps the stored one.
    Q_INVOKABLE QString saveAccount(const QString& service, const QString& user, const QString& password);
    Q_INVOKABLE void clearAccount();
    // HamAlert.org telnet login; empty password keeps the stored one, empty user clears both.
    Q_INVOKABLE QString saveHamAlert(const QString& user, const QString& password);
    // hams.at API key (Settings page on hams.at); empty clears it. Never shown back.
    Q_INVOKABLE QString saveHamsatKey(const QString& key);

signals:
    void stationChanged();
    void meshChanged(); // only when the Meshtastic node host or port actually changed
    void aprsChanged(); // only when the APRS-IS server, port or radius actually changed
    // Each only when that device's endpoint actually changed.
    void radioChanged();
    void rotatorChanged();
    void adsbChanged();
    void gpsChanged();
    void appearanceChanged();
    void optionsChanged();
    void configsChanged();
    void accountsChanged();
    void panesChanged();
    void demoModeChanged();

private:
    QString assignPane(int slot, const QString& widget_id, bool persist);
    void loadOptions();
    std::unique_ptr<CredentialStore> secrets_;
    QString secretKey(const QString& store_key) const;
    void loadAccounts();
    QString writeAccounts();

    QString config_dir_, station_path_, load_error_;
    StationSettings station_{};  // value-init: coordinates are 0, never indeterminate, when no station file exists
    bool configured_ = false;
    QString cluster_host_;
    int cluster_port_ = 7300;
    int cluster_ssid_ = 0;
    QStringList cluster_commands_;
    QStringList watch_list_;
    QString adif_path_;
    QString ota_program_, ota_band_, ota_mode_;
    int ota_max_age_min_ = 0;
    QString ota_sort_;  // "", "freq" or "dist" (empty = newest first)
    QString aprs_host_ = "rotate.aprs2.net";
    int psk_max_km_ = 0;
    QString psk_match_;
    bool psk_wspr_ = false;
    bool psk_rbn_ = false;
    int aprs_port_ = 14580, aprs_radius_km_ = 100;
    QString mesh_host_;
    int mesh_port_ = 4403;
    QString radio_host_ = "127.0.0.1", rotator_host_ = "127.0.0.1", adsb_host_ = "127.0.0.1", gps_host_ = "127.0.0.1";
    int radio_port_ = 4532, rotator_port_ = 4533, adsb_port_ = 8080, gps_port_ = 2947;
    QString gps_device_;
    int gps_baud_ = 9600;
    QString saveEndpoint(const char* group, const char* name, const QString& host, int port, QString& host_out, int& port_out, bool& changed);
    int udp_port_ = 0;
    bool udp_lan_ = false;
    QString udp_group_;
    bool udp_by_me_ = false;
    bool udp_sets_dx_ = false;
    bool dx_override_ = false;
    hamclock::Location saved_dx_{};  // the persisted DX while an override is active
    StationSettings persistable() const;
    QStringList map_hidden_;
    QHash<QString, int> map_label_;
    int map_min_label_px_ = 0;
    bool imperial_ = false;
    int date_order_ = 0;
    bool clock_12h_ = false;
    bool week_sunday_ = false;
    int banner_style_ = 0;
    int banner_bg_ = 0;
    QString banner_title_;
    QString dx_zone_override_;
    int overscan_ = 0;
    int text_scale_ = 100;
    int accent_style_ = 0;
    QString map_size_ = "large";
    QString orientation_lock_ = "auto";
    int big_clock_face_ = 0;
    int blank_idle_ = 0;
    int blank_off_ = -1;
    int blank_on_ = -1;
    int blank_days_ = 0x7F;
    bool demo_mode_ = false;
    QList<int> rotations_;
    QList<QStringList> rotation_sets_;
    QStringList panes_;   // empty entry = default for that slot
    QString storeAccount(const QString& service, const QString& user, const QString& secret);
    // Users by service (qrz, hamqth, hamalert); passwords and the hams.at key by service (+ hamsat).
    QHash<QString, QString> account_user_, account_secret_;
};

} // namespace hamclock
