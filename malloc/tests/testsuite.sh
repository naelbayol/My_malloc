#!/bin/bash

LIB="$(realpath "${1:-./libmalloc.so}")"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
PASS=0
FAIL=0

result()
{
    if [ "$2" -eq 0 ]; then
        PASS=$((PASS + 1))
        echo "[ OK ] $1"
    else
        FAIL=$((FAIL + 1))
        echo "[ KO ] $1"
    fi
}

run_c()
{
    {
        echo '#include <errno.h>'
        echo '#include <stdint.h>'
        echo '#include <stdlib.h>'
        echo '#include <string.h>'
        echo '#define CHECK(Cond) if (!(Cond)) return 1'
        cat
    } > "$TMP/test.c"
    gcc -w -o "$TMP/test" "$TMP/test.c" && LD_PRELOAD="$LIB" "$TMP/test"
    result "$1" $?
}

run_cmd()
{
    expected="$("$@" 2>&1)"
    got="$(LD_PRELOAD="$LIB" "$@" 2>&1)"
    [ "$got" = "$expected" ]
    result "$*" $?
}

run_c "malloc : ecrire et relire" << 'EOF'
int main(void)
{
    char *p = malloc(100);
    CHECK(p != NULL);
    memset(p, 'a', 100);
    CHECK(p[0] == 'a' && p[99] == 'a');
    return 0;
}
EOF

run_c "malloc : alignement sur 16" << 'EOF'
int main(void)
{
    for (size_t size = 1; size < 3000; size++)
        CHECK((uintptr_t)malloc(size) % 16 == 0);
    return 0;
}
EOF

run_c "malloc : blocs sans chevauchement" << 'EOF'
int main(void)
{
    char *a = malloc(100);
    char *b = malloc(100);
    memset(a, 'a', 100);
    memset(b, 'b', 100);
    CHECK(a[99] == 'a' && b[0] == 'b');
    return 0;
}
EOF

run_c "malloc : taille trop grande renvoie NULL" << 'EOF'
int main(void)
{
    CHECK(malloc(SIZE_MAX) == NULL && errno == ENOMEM);
    return 0;
}
EOF

run_c "free : NULL et reutilisation" << 'EOF'
int main(void)
{
    free(NULL);
    void *keep = malloc(16);
    void *p = malloc(64);
    free(p);
    CHECK(malloc(64) == p);
    free(keep);
    return 0;
}
EOF

run_c "free : fusion des blocs voisins" << 'EOF'
int main(void)
{
    char *a = malloc(1000);
    char *b = malloc(1000);
    char *keep = malloc(16);
    free(a);
    free(b);
    CHECK(malloc(2000) == a);
    free(keep);
    return 0;
}
EOF

run_c "calloc : zone a zero et depassement" << 'EOF'
int main(void)
{
    char *p = calloc(1000, 4);
    for (int i = 0; i < 4000; i++)
        CHECK(p[i] == 0);
    CHECK(calloc(SIZE_MAX, 2) == NULL);
    return 0;
}
EOF

run_c "realloc : NULL et taille 0" << 'EOF'
int main(void)
{
    char *p = realloc(NULL, 50);
    CHECK(p != NULL);
    CHECK(realloc(p, 0) == NULL);
    return 0;
}
EOF

run_c "realloc : agrandir et reduire gardent les donnees" << 'EOF'
int main(void)
{
    char *p = malloc(100);
    memset(p, 'x', 100);
    p = realloc(p, 5000);
    CHECK(p != NULL && p[99] == 'x');
    p = realloc(p, 10);
    CHECK(p != NULL && p[9] == 'x');
    return 0;
}
EOF

cd "$TMP" || exit 1
cp "$LIB" lib.so
run_cmd factor 20 30 40 50 60 70 80 90
run_cmd ls -la /usr/bin
run_cmd od lib.so
run_cmd find /usr/include

echo "Resultat : $PASS OK, $FAIL KO"
[ "$FAIL" -eq 0 ]
