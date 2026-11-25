#include <celix_framework_factory.h>
#include <stdio.h>

int main(void) {
    celix_framework_t *fw = celix_frameworkFactory_createFramework(NULL);
    if (fw == NULL) {
        fprintf(stderr, "framework_smoke_test: FAILED to create/start framework\n");
        return 1;
    }
    celix_frameworkFactory_destroyFramework(fw);
    fprintf(stderr, "framework_smoke_test: framework created, started, and destroyed\n");
    return 0;
}
