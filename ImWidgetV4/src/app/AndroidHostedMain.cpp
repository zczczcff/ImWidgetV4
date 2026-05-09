#include <android_native_app_glue.h>
#include <imwidgetv4/app/ApplicationHost.h>

void android_main(struct android_app* app)
{
    ImWidgetV4::RunHostedAndroidApplication(app);
}
