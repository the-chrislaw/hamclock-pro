#!/usr/bin/env python3
"""Create (or delete) an iOS, tvOS or macOS App Store provisioning profile through the App Store
Connect API, for CI archives signed manually with an imported Apple Distribution
certificate. Needs `pip install pyjwt cryptography`. Secrets come from the
environment and are never printed.

  asc_profile.py create <bundle-id> <cert.pem> <profile-name> <out.mobileprovision> [profile-type]
      profile-type defaults to IOS_APP_STORE (TVOS_APP_STORE for Apple TV, MAC_APP_STORE for the Mac App Store);
      prints the profile id and uuid as "ID UUID"
  asc_profile.py delete <profile-id>

Env: ASC_KEY_FILE (.p8 path), ASC_KEY_ID, ASC_ISSUER_ID.
"""
import base64
import json
import os
import re
import sys
import time
import urllib.error
import urllib.request

import jwt

API = "https://api.appstoreconnect.apple.com/v1"


def token():
    with open(os.environ["ASC_KEY_FILE"], "rb") as f:
        key = f.read()
    now = int(time.time())
    return jwt.encode(
        {"iss": os.environ["ASC_ISSUER_ID"], "iat": now, "exp": now + 600,
         "aud": "appstoreconnect-v1"},
        key, algorithm="ES256",
        headers={"kid": os.environ["ASC_KEY_ID"], "typ": "JWT"})


def call(method, path, body=None):
    req = urllib.request.Request(
        API + path, method=method,
        data=json.dumps(body).encode() if body is not None else None,
        headers={"Authorization": "Bearer " + token(), "Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=60) as r:
            raw = r.read()
            return json.loads(raw) if raw else {}
    except urllib.error.HTTPError as e:
        sys.exit("ASC API %s %s -> HTTP %d: %s" % (method, path.split("?")[0], e.code, e.read()[:400]))


def pem_to_der_b64(path):
    pem = open(path).read()
    m = re.search(r"-----BEGIN CERTIFICATE-----(.*?)-----END CERTIFICATE-----", pem, re.S)
    return re.sub(r"\s+", "", m.group(1))


def create(bundle_id, cert_pem, name, out, profile_type="IOS_APP_STORE"):
    want = pem_to_der_b64(cert_pem)
    certs = call("GET", "/certificates?filter[certificateType]=IOS_DISTRIBUTION,DISTRIBUTION&limit=200")["data"]
    cert = next((c for c in certs if c["attributes"].get("certificateContent") == want), None)
    if cert is None:
        sys.exit("The imported Apple Distribution certificate is not registered for this team")
    bundles = call("GET", "/bundleIds?filter[identifier]=%s&limit=50" % bundle_id)["data"]
    bundle = next((b for b in bundles if b["attributes"]["identifier"] == bundle_id), None)
    if bundle is None:
        sys.exit("Bundle ID %s is not registered for this team" % bundle_id)
    prof = call("POST", "/profiles", {"data": {
        "type": "profiles",
        "attributes": {"name": name, "profileType": profile_type},
        "relationships": {
            "bundleId": {"data": {"type": "bundleIds", "id": bundle["id"]}},
            "certificates": {"data": [{"type": "certificates", "id": cert["id"]}]}}}})["data"]
    with open(out, "wb") as f:
        f.write(base64.b64decode(prof["attributes"]["profileContent"]))
    print(prof["id"], prof["attributes"]["uuid"])


def main(argv):
    if len(argv) in (6, 7) and argv[1] == "create":
        create(*argv[2:])
    elif len(argv) == 3 and argv[1] == "delete":
        call("DELETE", "/profiles/" + argv[2])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main(sys.argv)
