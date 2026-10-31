/// Values preserved from the working Android RainMaker application.
///
/// Keep the redirect URI and client id together with the Android package id
/// until a new OAuth client has been registered in RainMaker.
abstract final class AppConfig {
  static const appName = 'TVT RainMaker';
  static const apiBaseUrl = 'https://api.rainmaker.espressif.com';
  static const authBaseUrl = 'https://auth.rainmaker.espressif.com/oauth2';
  static const oauthClientId = '60i6kac5f9rjuetqnq5mnmaqv6';
  static const redirectUri = 'rainmaker://com.espressif.rainmaker/success';

  static const bleNamePrefix = 'PROV_';
  static const bleServiceUuid = '0000ffff-0000-1000-8000-00805f9b34fb';
  static const cloudUserAssociationEndpoint = 'cloud_user_assoc';
}
