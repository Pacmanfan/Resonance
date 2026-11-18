#include "webui.h"

#include <Arduino.h>
#include <WebServer.h>

#include "../radio/wifi_link.h"
#include "../storage/sdcard.h"
#include "../storage/settings_store.h"
#include "../version.h"
#include "rc_paths.h"
#include "webpage.h"

namespace webui {

const char kUser[] = "resonance";

namespace {

WebServer& server() {
    static WebServer instance(80);  // built on first use, after the scheduler started
    return instance;
}

rc::Settings* settings = nullptr;
bool restartWanted = false;
bool restartToSetup = false;
uint32_t requests = 0;
uint32_t uploads = 0;

// One upload at a time. The file is written under a temporary name and gets
// its real name only when it has arrived complete and has been recognised as
// sound (RES-WEB-004, RES-WEB-005).
struct Upload {
    fs::File file;
    char finalPath[200];
    char tempPath[208];
    const char* error;   // nullptr while all is well
    int status;          // HTTP status to answer with on error
    uint32_t received;
    bool sniffed;
    bool active;
} upload = {fs::File(), "", "", nullptr, 200, 0, false, false};

// Every request must be authenticated (RES-WEB-006). Digest authentication:
// the password itself does not cross the network.
bool authorised() {
    ++requests;
    if (server().authenticate(kUser, settings->apPass)) return true;
    server().requestAuthentication(DIGEST_AUTH, "Resonance", "Log in with the password shown on the device");
    return false;
}

void sendJson(int code, const char* body) {
    server().sendHeader("Cache-Control", "no-store");
    server().send(code, "application/json", body);
}

void sendError(int code, const char* message) {
    char body[160];
    snprintf(body, sizeof(body), "{\"error\":\"%s\"}", message);  // messages are our own literals
    sendJson(code, body);
}

// Builds the absolute path of a folder of the sound library from the "dir"
// argument. Refuses anything that is not a safe relative path.
bool folderFromRequest(char* out, size_t capacity) {
    const String dir = server().arg("dir");
    if (!rc::isSafeRelativePath(dir.c_str())) return false;
    if (dir.length() == 0) snprintf(out, capacity, "%s", sdcard::kAudioRoot);
    else snprintf(out, capacity, "%s/%s", sdcard::kAudioRoot, dir.c_str());
    return true;
}

void handlePage() {
    if (!authorised()) return;
    server().sendHeader("Cache-Control", "no-cache");
    server().send_P(200, "text/html; charset=utf-8", kPage);
}

void handleStatus() {
    if (!authorised()) return;
    const sdcard::Info sd = sdcard::info();
    char name[40], ssid[80], network[80], body[420];
    rc::jsonEscape(settings->hostname, name, sizeof(name));
    rc::jsonEscape(settings->wifiSsid, ssid, sizeof(ssid));
    rc::jsonEscape(wifilink::networkName(), network, sizeof(network));
    // The Wi-Fi password is never part of any answer (RES-NET-004).
    snprintf(body, sizeof(body),
             "{\"name\":\"%s\",\"version\":\"%s\",\"mode\":\"setup\",\"sd\":%s,\"totalMb\":%lu,\"freeMb\":%lu,"
             "\"network\":\"%s\",\"ssid\":\"%s\",\"heap\":%lu}",
             name, RESONANCE_VERSION, sd.mounted ? "true" : "false", static_cast<unsigned long>(sd.totalMb),
             static_cast<unsigned long>(sd.freeMb), network, ssid, static_cast<unsigned long>(ESP.getFreeHeap()));
    sendJson(200, body);
}

void sendEntry(const char* name, bool directory, uint32_t size, void* context) {
    bool* first = static_cast<bool*>(context);
    char escaped[sdcard::kNameLength * 6 + 1], item[sizeof(escaped) + 48];
    if (!rc::jsonEscape(name, escaped, sizeof(escaped))) return;
    snprintf(item, sizeof(item), "%s{\"n\":\"%s\",\"d\":%d,\"s\":%lu}", *first ? "" : ",", escaped, directory ? 1 : 0,
             static_cast<unsigned long>(size));
    *first = false;
    server().sendContent(item);
}

void handleFiles() {
    if (!authorised()) return;
    char folder[200];
    if (!folderFromRequest(folder, sizeof(folder))) return sendError(400, "bad folder");
    if (!sdcard::info().mounted) return sendError(503, "no SD card");
    if (!sdcard::exists(folder)) return sendError(404, "no such folder");
    // Sent piece by piece: a folder may hold more entries than fit in RAM.
    server().sendHeader("Cache-Control", "no-store");
    server().setContentLength(CONTENT_LENGTH_UNKNOWN);
    server().send(200, "application/json", "");
    server().sendContent("{\"entries\":[");
    bool first = true;
    sdcard::visit(folder, sendEntry, &first);
    server().sendContent("]}");
    server().sendContent("");
}

void handleDelete() {
    if (!authorised()) return;
    char folder[200], path[280];
    const String name = server().arg("name");
    if (!folderFromRequest(folder, sizeof(folder)) || !rc::isSafeName(name.c_str())) return sendError(400, "bad name");
    snprintf(path, sizeof(path), "%s/%s", folder, name.c_str());
    if (!sdcard::exists(path)) return sendError(404, "not found");
    if (!sdcard::remove(path)) return sendError(409, "could not delete (a folder must be empty)");
    sdcard::refresh();
    sendJson(200, "{\"ok\":true}");
}

void handleMkdir() {
    if (!authorised()) return;
    char folder[200], path[280];
    const String name = server().arg("name");
    if (!folderFromRequest(folder, sizeof(folder)) || !rc::isSafeName(name.c_str())) return sendError(400, "bad name");
    snprintf(path, sizeof(path), "%s/%s", folder, name.c_str());
    if (strlen(path) - strlen(sdcard::kAudioRoot) > rc::kMaxRelativePath) return sendError(400, "path too long");
    if (sdcard::exists(path)) return sendError(409, "already exists");
    if (!sdcard::makeDirectory(path)) return sendError(500, "could not create the folder");
    sendJson(200, "{\"ok\":true}");
}

void failUpload(int status, const char* message) {
    if (upload.error == nullptr) {
        upload.error = message;
        upload.status = status;
    }
    if (upload.file) upload.file.close();
    if (upload.tempPath[0] != '\0') sdcard::remove(upload.tempPath);
}

// Called by the server for each piece of an uploaded file.
void handleUploadData() {
    HTTPUpload& part = server().upload();
    if (part.status == UPLOAD_FILE_START) {
        upload.error = nullptr;
        upload.status = 200;
        upload.received = 0;
        upload.sniffed = false;
        upload.active = true;
        upload.finalPath[0] = upload.tempPath[0] = '\0';
        if (!server().authenticate(kUser, settings->apPass)) return failUpload(401, "not logged in");
        char folder[200];
        // The browser's own idea of the file's type is ignored; its name is
        // used only after it has passed the rules.
        if (!folderFromRequest(folder, sizeof(folder))) return failUpload(400, "bad folder");
        if (!rc::isSoundFileName(part.filename.c_str())) return failUpload(400, "only .mp3 and .wav files with plain names");
        if (!sdcard::info().mounted) return failUpload(503, "no SD card");
        if (!sdcard::exists(folder)) return failUpload(404, "no such folder");
        const uint32_t sizeMb = static_cast<uint32_t>(server().arg("size").toInt() >> 20);
        if (sizeMb + 2 > sdcard::info().freeMb) return failUpload(507, "not enough space on the card");
        snprintf(upload.finalPath, sizeof(upload.finalPath), "%s/%s", folder, part.filename.c_str());
        snprintf(upload.tempPath, sizeof(upload.tempPath), "%s.part", upload.finalPath);
        if (sdcard::exists(upload.finalPath) && server().arg("overwrite") != "1") {
            upload.tempPath[0] = '\0';
            return failUpload(409, "a file of that name exists");
        }
        upload.file = sdcard::openForWrite(upload.tempPath);
        if (!upload.file) return failUpload(500, "could not create the file");
    } else if (part.status == UPLOAD_FILE_WRITE) {
        if (upload.error != nullptr || !upload.file) return;
        if (!upload.sniffed) {
            // The first piece must look like the sound format the name claims.
            upload.sniffed = true;
            const rc::SoundKind kind = rc::sniffSound(part.buf, part.currentSize);
            if (!rc::nameMatchesKind(upload.finalPath, kind)) return failUpload(415, "the file is not the sound format its name says");
        }
        if (upload.file.write(part.buf, part.currentSize) != part.currentSize) return failUpload(507, "the card is full or failed");
        upload.received += part.currentSize;
    } else if (part.status == UPLOAD_FILE_END) {
        if (upload.error != nullptr) return;
        if (upload.file) upload.file.close();
        if (!upload.sniffed || upload.received == 0) return failUpload(400, "the file is empty");
        // Only now does the file take its real name.
        if (sdcard::exists(upload.finalPath)) sdcard::remove(upload.finalPath);
        if (!sdcard::rename(upload.tempPath, upload.finalPath)) return failUpload(500, "could not store the file");
        upload.tempPath[0] = '\0';
        ++uploads;
    } else if (part.status == UPLOAD_FILE_ABORTED) {
        failUpload(499, "upload interrupted");
    }
}

// Called when the upload request is complete.
void handleUploadDone() {
    ++requests;
    if (!upload.active) return sendError(400, "no file");
    upload.active = false;
    if (upload.error != nullptr) {
        if (upload.status == 401) return server().requestAuthentication(DIGEST_AUTH, "Resonance");
        return sendError(upload.status, upload.error);
    }
    sdcard::refresh();
    sendJson(200, "{\"ok\":true}");
}

void handleWifi() {
    if (!authorised()) return;
    const String ssid = server().arg("ssid"), password = server().arg("password"), hostname = server().arg("hostname");
    if (ssid.length() >= sizeof(settings->wifiSsid) || password.length() >= sizeof(settings->wifiPass)) {
        return sendError(400, "name or password too long");
    }
    if (ssid.length() > 0 && password.length() > 0 && password.length() < 8) return sendError(400, "a Wi-Fi password has at least 8 characters");
    if (hostname.length() > 0 && !rc::isValidHostname(hostname.c_str())) return sendError(400, "device name: letters, digits and '-' only");
    if (!rc::isPrintableField(ssid.c_str(), sizeof(settings->wifiSsid)) ||
        !rc::isPrintableField(password.c_str(), sizeof(settings->wifiPass))) {
        return sendError(400, "plain characters only");
    }
    strlcpy(settings->wifiSsid, ssid.c_str(), sizeof(settings->wifiSsid));
    strlcpy(settings->wifiPass, ssid.length() > 0 ? password.c_str() : "", sizeof(settings->wifiPass));
    if (hostname.length() > 0) strlcpy(settings->hostname, hostname.c_str(), sizeof(settings->hostname));
    if (!storage::save(*settings)) return sendError(500, "could not store the settings");
    sendJson(200, "{\"ok\":true,\"restarting\":true}");
    restartWanted = true;
    restartToSetup = true;  // stay in Setup: the new network is tried there
}

void handleRestart() {
    if (!authorised()) return;
    const String mode = server().arg("mode");
    if (mode != "play" && mode != "setup") return sendError(400, "mode must be play or setup");
    sendJson(200, "{\"ok\":true,\"restarting\":true}");
    restartWanted = true;
    restartToSetup = mode == "setup";
}

void handleNotFound() {
    ++requests;
    sendError(404, "not found");
}

}  // namespace

void begin(rc::Settings& deviceSettings) {
    settings = &deviceSettings;
    sdcard::removeLeftovers(sdcard::kAudioRoot);  // uploads cut off by a power loss
    WebServer& s = server();
    s.on("/", HTTP_GET, handlePage);
    s.on("/api/status", HTTP_GET, handleStatus);
    s.on("/api/files", HTTP_GET, handleFiles);
    s.on("/api/delete", HTTP_POST, handleDelete);
    s.on("/api/mkdir", HTTP_POST, handleMkdir);
    s.on("/api/upload", HTTP_POST, handleUploadDone, handleUploadData);
    s.on("/api/wifi", HTTP_POST, handleWifi);
    s.on("/api/restart", HTTP_POST, handleRestart);
    s.onNotFound(handleNotFound);
    s.begin();
}

void tick() {
    if (settings != nullptr) server().handleClient();
}

bool takeRestartRequest(bool* toSetup) {
    if (!restartWanted) return false;
    restartWanted = false;
    *toSetup = restartToSetup;
    return true;
}

uint32_t requestCount() { return requests; }
uint32_t uploadCount() { return uploads; }

}  // namespace webui
