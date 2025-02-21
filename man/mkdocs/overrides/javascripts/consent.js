var consent = __md_get("__consent")
console.debug(consent);
if (consent && consent.analytics) {
    /* The user accepted the cookie */
    console.debug("User accepted Analytics");
    _paq.push([ function() {
        if (this.isUserOptedOut()) {
            // ... change form to say user is currently opted out ...
            console.debug("Switching form opt-out to opt-in");
            _paq.push([ 'forgetUserOptOut' ]);
        }
    } ]);
} else {
    /* The user rejected the cookie */
    _paq.push([ 'optUserOut' ]); // Matomo opt-out
    console.debug("User rejected Analytics");
}
