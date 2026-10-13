#include "credential_store.hpp"

#import <Foundation/Foundation.h>
#import <Security/Security.h>

namespace hamclock {

namespace {
NSMutableDictionary* baseQuery(const QString& service, const QString& key) {
    return [NSMutableDictionary dictionaryWithObjectsAndKeys:
        (__bridge id)kSecClassGenericPassword, (__bridge id)kSecClass,
        service.toNSString(), (__bridge id)kSecAttrService,
        key.toNSString(), (__bridge id)kSecAttrAccount,
        nil];
}
} // namespace

KeychainCredentialStore::KeychainCredentialStore(QString service) : m_service(std::move(service)) {}

bool KeychainCredentialStore::set(const QString& key, const QString& value) {
    NSMutableDictionary* q = baseQuery(m_service, key);
    SecItemDelete((__bridge CFDictionaryRef)q);
    q[(__bridge id)kSecValueData] = [value.toNSString() dataUsingEncoding:NSUTF8StringEncoding];
    q[(__bridge id)kSecAttrAccessible] = (__bridge id)kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly;
    return SecItemAdd((__bridge CFDictionaryRef)q, nullptr) == errSecSuccess;
}

QString KeychainCredentialStore::get(const QString& key) const {
    NSMutableDictionary* q = baseQuery(m_service, key);
    q[(__bridge id)kSecReturnData] = @YES;
    q[(__bridge id)kSecMatchLimit] = (__bridge id)kSecMatchLimitOne;
    CFTypeRef out = nullptr;
    if (SecItemCopyMatching((__bridge CFDictionaryRef)q, &out) != errSecSuccess || !out) {
        return {};
    }
    NSData* data = (__bridge_transfer NSData*)out;
    NSString* s = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    return s ? QString::fromNSString(s) : QString();
}

bool KeychainCredentialStore::remove(const QString& key) {
    const OSStatus st = SecItemDelete((__bridge CFDictionaryRef)baseQuery(m_service, key));
    return st == errSecSuccess || st == errSecItemNotFound;
}

bool KeychainCredentialStore::available() const {
    const QString probe = QStringLiteral("__hamclock_probe__");
    KeychainCredentialStore self(m_service);
    const bool ok = self.set(probe, QStringLiteral("1"));
    self.remove(probe);
    return ok;
}

std::unique_ptr<CredentialStore> makeDefaultCredentialStore() {
    return std::make_unique<KeychainCredentialStore>(QStringLiteral("org.hamclockpro.app"));
}

} // namespace hamclock
