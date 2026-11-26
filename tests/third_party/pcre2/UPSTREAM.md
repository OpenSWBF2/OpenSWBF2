# PCRE2

- Release: PCRE2 10.49, tag `pcre2-10.49`
- Archive: https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.49/pcre2-10.49.tar.gz
- Archive SHA-256: `929f0b20e62879252a15886b06c89f1edef61a363cbd5826fb041080a5e557ae`
- Signature: `pcre2-10.49.tar.gz.sig` from the same release, a good signature by subkey `BACF71F10404D5761C09D392021DE40BFB63B406` of key `A95536204A3BB489715231282A98E77EB6F24CA8` (Nicholas Wilson), the release key named in the release's `SECURITY.md`
- Licence: `LICENCE.md`, BSD-3-Clause WITH PCRE2-exception

`src` holds the 8-bit library sources that `NON-AUTOTOOLS-BUILD` lists and the headers they include without JIT. `pcre2.h` is `pcre2.h.generic`, `pcre2_chartables.c` is `pcre2_chartables.c.dist` and `config.h` is `config.h.generic` with `SUPPORT_PCRE2_8` defined, as step 1 of `NON-AUTOTOOLS-BUILD` asks. No other file differs from the archive.

`tests\OpenSWBF2Tests.vcxproj` builds them with `HAVE_CONFIG_H`, `PCRE2_CODE_UNIT_WIDTH=8` and `PCRE2_STATIC`. JIT is not built.
