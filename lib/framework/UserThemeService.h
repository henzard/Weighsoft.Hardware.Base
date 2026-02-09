#ifndef UserThemeService_h
#define UserThemeService_h

#include <SecurityManager.h>
#include <SecuritySettingsService.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

#define USER_THEME_ENDPOINT_PATH "/rest/userTheme"

#if FT_ENABLED(FT_SECURITY)

class UserThemeService {
 public:
  UserThemeService(AsyncWebServer* server,
                   SecurityManager* securityManager,
                   SecuritySettingsService* securitySettingsService)
      : _server(server), _securityManager(securityManager), _securitySettingsService(securitySettingsService) {
    // GET: Read current user's theme
    _server->on(
        USER_THEME_ENDPOINT_PATH,
        HTTP_GET,
        _securityManager->wrapRequest(
            [this](AsyncWebServerRequest* request) { handleGetTheme(request); },
            AuthenticationPredicates::IS_AUTHENTICATED));

    // PUT: Update current user's theme
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler(
        USER_THEME_ENDPOINT_PATH,
        _securityManager->wrapCallback(
            [this](AsyncWebServerRequest* request, JsonVariant& json) { handlePutTheme(request, json); },
            AuthenticationPredicates::IS_AUTHENTICATED));
    _server->addHandler(handler);
  }

 private:
  AsyncWebServer* _server;
  SecurityManager* _securityManager;
  SecuritySettingsService* _securitySettingsService;

  void handleGetTheme(AsyncWebServerRequest* request) {
    Authentication auth = _securityManager->authenticateRequest(request);

    if (!auth.authenticated) {
      request->send(401);
      return;
    }

    // Find user and return theme
    _securitySettingsService->read([&](SecuritySettings& settings) {
      for (const User& user : settings.users) {
        if (user.username == auth.user->username) {
          AsyncJsonResponse* response = new AsyncJsonResponse(false);
          JsonObject root = response->getRoot();
          root["theme"] = user.themePreference;
          response->setLength();
          request->send(response);
          return;
        }
      }
      request->send(404);  // User not found
    });
  }

  void handlePutTheme(AsyncWebServerRequest* request, JsonVariant& json) {
    if (!json.is<JsonObject>()) {
      request->send(400);
      return;
    }

    JsonObject jsonObject = json.as<JsonObject>();
    String newTheme = jsonObject["theme"] | "light";

    // Validate theme value
    if (newTheme != "light" && newTheme != "dark") {
      request->send(400);
      return;
    }

    Authentication auth = _securityManager->authenticateRequest(request);

    // Update user's theme preference
    bool updated = false;
    _securitySettingsService->update(
        [&](SecuritySettings& settings) {
          for (User& user : settings.users) {
            if (user.username == auth.user->username) {
              user.themePreference = newTheme;
              updated = true;
              return StateUpdateResult::CHANGED;
            }
          }
          return StateUpdateResult::UNCHANGED;
        },
        "http");

    if (updated) {
      AsyncJsonResponse* response = new AsyncJsonResponse(false);
      JsonObject root = response->getRoot();
      root["theme"] = newTheme;
      response->setLength();
      request->send(response);
    } else {
      request->send(404);
    }
  }
};

#endif  // end FT_ENABLED(FT_SECURITY)
#endif  // end UserThemeService_h
