/*
 *  app-core
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>, Sewook Park <sewook7.park@samsung.com>, Jaeho Lee <jaeho81.lee@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

 
/**
 @ingroup SLP_PG
 @defgroup SLP_PG_APPCORE Application Model and Appcore
 @{

<h1 class="pg">Introduction</h1>

<h2 class="pg">Purpose of this document</h2>
 This document demonstrates the basic steps needed to develop application(EFL or GTK) using appcore module in the SLP(Samsung Linux Platform). The document provides a brief introduction to the appcore architecture and explains application life-cycle of appcore. Programmers should develop their applications(EFL or GTK) based on the appcore module. The sample applications can be studied and can be used to develop other applications.

<h2 class="pg">Features</h2>
- Support EFL and GTK(including STK) application's basic function
- Support internalization
- Support rotation function
- Support power state function



<h1 class="pg">Components of Appcore</h3>
@image html SLP_Appcore_PG_overview.png

- Appcore EFL(libappcore-efl.so)
  - It provides appcore_efl_main() which includes elm_init(), elm_run(), and other initializations for EFL application
  - Refer to appcore-efl.h
- Appcore common(libappcore-common.so)
  - It provides useful rotation function to control sensor’s rotation operation in application
  - It provides useful function to process internalization based on GNU gettext
  - It provides useful function to control power-state in application
  - Refer to appcore-common.h
- Libraries used by Appcore
  - EFL and GTK(STK) is a graphic widget library
  - Sensor for supporting rotation
  - AUL for application's life-cycle
  - RUA for task manager
  - Vconf for system events (such as low battery, low memory).
@}
 @defgroup SLP_PG_APPCORE1 1.Application's life-cycle
 @ingroup SLP_PG_APPCORE
 @{

<h1 class="pg">Application's life-cycle</h2>
Appcore provides basic life-cycle like the following to manage application in SLP
<table>
  <tr>
    <td> @image html SLP_Appcore_PG_lifecycle.png </td>
    <td>
      <table>
        <tr>
	  <th>Operation</th>
	  <th>Description</th>
	</tr>
	<tr>
	  <td>CREATE</td>
	  <td>Called once before the main loop. Initialize the application such as window creation, data structure allocation, and etc.</td>
	</tr>
	<tr>
	  <td>RESET</td>
	  <td>Called at the first idler and every "relaunch" message. Reset the application states and data structures.</td>
	</tr>
	<tr>
	  <td>PAUSE</td>
	  <td>Called when the entire window in this application are invisible. Recommend to suspend the actions related to the visibility.</td>
	</tr>
	<tr>
	  <td>RESUME</td>
	  <td>Called when one of the windows in this application is visible. Resume the paused actions.</td>
	</tr>
	<tr>
	  <td>TERMINATE</td>
	  <td>Called once after the main loop. Release the resources.</td>
	</tr>
      </table> 
    </td>
  </tr>
</table>
Call the main function (appcore_efl_main() with struct appcore_ops filled with the proper function pointers. The functions are called by Appcore at the proper situation.
@code
struct appcore_ops {
        void *data; // Callback data
        int (*create)(void *); // Called before main loop 
        int (*terminate)(void *); // Called after main loop 
        int (*pause)(void *); // Called when every window goes back
        int (*resume)(void *); // Called when any window comes on top
        int (*reset)(bundle *, void *); // Called at the first idler and every relaunching
};
@endcode

@}
 @defgroup SLP_PG_APPCORE2 2.Appcore event handling
 @ingroup SLP_PG_APPCORE
 @{

<h1 class="pg">Appcore event handling</h2>
An Application should perform an action when a system change occurs, such as the battery level goes low. Appcore provides APIs to handle the system changes notified by other frameworks and managers. The following figure shows the relationship between events and callback functions.

@image html SLP_Appcore_PG_events.png 

Appcore provides appcore_set_event_callback() to handle events from the system. If you set the callback function, it will be called when the event received. Otherwise, Appcore does default behavior which is defined as following.
<table>
  <tr>
    <th>Event</th>
    <th>Description</th>
    <th>Default behavior</th>
  </tr>
  <tr>
    <td>APPCORE_EVENT_LOW_MEMORY</td>
    <td>A system memory goes low</td>
    <td>call malloc_trim() to enforce giving back the freed memory</td>
  </tr>
  <tr>
    <td>APPCORE_EVENT_LOW_BATTERY</td>
    <td>A system is out of battery</td>
    <td>Quit the main loop</td>
  </tr>
  <tr>
    <td>APPCORE_EVENT_LANG_CHANGE</td>
    <td>A system's language setting is changed</td>
    <td>Reset the environment variable "LANG". This has influence on the function related to internationalization</td>
  </tr>
</table>


@}
 @defgroup SLP_PG_APPCORE3 3.Internationalization
 @ingroup SLP_PG_APPCORE
 @{

<h1 class="pg">Internationalization</h3>
SLP platform uses GNU gettext for Internationalization.
The process of using gettext is as follows:
-# Marking any phrases to be translated using gettext(“phrase”) or gettext_noop(“phrase”)
-# Generating a <I>po</I> file by extracting marked phrases using a <I>xgettext</I> command.
-# Modifying the <I>po</I> file and entering a translated phrase for the corresponding phrase.
-# Compiling a <I>po</I> file into a <I>mo</I>  file using a <I>msgfmt</I> command.
-# Installing <I>mo</I> files into proper locale directories.
-# gettext() returns a properly translated phrase while an application is running.

For example, we have the following source code
@code
printf(“%s\n”, gettext(“Hello”));
@endcode

Then, as you can see, we can have the following po file using xgettext,
@code
#: hello.c:41
msgid "Hello"
msgstr ""
@endcode

And we can enter a translated phrase as the following,
@code
#: hello.c:41
msgid "Hello"
msgstr "안녕하세요" // Hello in Korean
@endcode

Finally we generate and install a mo file. In doing so, an application will print out "안녕하세요" instead of "Hello", if the applied language is Korean.

appcore_set_i18n() has been introduced to achieve an internationalization which requires information on a mo file's name and its installed directory.
@code
int appcore_set_i18n(const char *domainname, const char *dirname)
@endcode

The first parameter, <I>domainname</I>, a mo file’s name and the second one, <I>dirname</I>, is the directory.<br>
In general, a mo file will be installed in the following fashion, <I>dirname/locale/category/domainname.mo</I>. <I>locale</I> is a locale’s name, such as ‘ko’ and ‘ja’, and category is LC_MESSAGES. For instance, a mo file has been installed in /usr/share/locale/ko/LC_MESSAGES/example.mo, and then <I>dirname</I> becomes /usr/share/locale.<br>
Typically, mo files will be installed in ${prefix}/share/locale.

Generally the following macros are defined for easy to use gettext()
@code
#define _(str) gettext(str)
#define gettext_noop(str) (str)
#define N_(str) gettext_noop(str)
@endcode

N_() macro only extracts any marked phrases using xgettext command, it will not do anything in the action. This macro is used to handle an array of phrases. Since gettext() cannot be called in declaration of an array, we let N_() macro extract phrases and then call gettext(). Let’s look at the following example.

@code
static const char *messages[] = {
	N_(“Hello”),
	N_(“World”)
};
…
printf(“Message: %s\n”, _(messages[0]);
…
@endcode

In doing so, we now can handle phrases in an array as same as we do for the following.

@code
printf(“Message: %s\n”, _(“Hello”);
@endcode

For more details, refer to GNU gettext manual (http://www.gnu.org/software/gettext/manual/gettext.html)

@}
 @defgroup SLP_PG_APPCORE4 4.Rotation
 @ingroup SLP_PG_APPCORE
 @{

<h1 class="pg">Rotation</h3>
An Application can display its UI in either landscape or portrait mode. The application registers to receive rotation events from the system with the appcore_set_rotation_cb() API. The system automatically calls the registered user's callback whenever the sensor framework detects that the current rotation status has changed. This continues until appcore_unset_rotation_cb() is called.

@code
enum appcore_rm {
        APPCORE_RM_UNKNOWN,
        APPCORE_RM_PORTRAIT_NORMAL , // Portrait mode
        APPCORE_RM_PORTRAIT_REVERSE , // Portrait upside down mode 
        APPCORE_RM_LANDSCAPE_NORMAL , // Left handed landscape mode
        APPCORE_RM_LANDSCAPE_REVERSE ,  // Right handed landscape mode 
};

int appcore_set_rotation_cb(int (*cb)(enum appcore_rm, void *), void *data);
int appcore_unset_rotation_cb(void);
int appcore_get_rotation_state(enum appcore_rm *curr);
@endcode

<I>enum appcore_rm</I> has portrait, portrait upside down, left handed landscape, and right handed landscape mode. The following is an each mode.
@image html SLP_Appcore_PG_rotation.png

When the registered callback is called, it receives the current mode state as the first argument. According to the received mode state, the application should rotate the window, resize window to changed screen size and composite the screen.

@}
 @defgroup SLP_PG_APPCORE5 5.Using code template
 @ingroup SLP_PG_APPCORE
 @{

<h1 class="pg">Using code template</h1>
We provide a code template for reference and convenience. You can generate a code template using the script "app-gen.sh" which is included in the <I>app-template</I> package.

The usage as follow:
@code
# app-gen.sh
Usage)	app-gen.sh dest app_name [EFL|GTK] 
	app_name does not support _ string because of debian usage."

ex) 
 EFL application: 
  # app-gen.sh ~/efl_app MyApp

 GTK application: 
  # app-gen.sh /home/app/gtk_app TestApp GTK

@endcode

Let's make a simple application using template
-# Install <I>app-template</I> package
@code
# apt-get install app-template
@endcode
-# Generate a code template
@code
# app-gen.sh ~/apps/simple simple EFL
@endcode
-# Build a package
@code
# cd ~/apps/simple
# dpkg-buildpackage -sa -rfakeroot
@endcode
@}
 @defgroup SLP_PG_APPCORE6 6.Example: EFL Apps
 @ingroup SLP_PG_APPCORE
 @{

<h1 class="pg">Example: EFL application</h1>

Header example
@code
12 #ifndef __APP_COMMON_H__
13 #define __APP_COMMON_H__
14 
15 #include <Elementary.h>
16 
17 #if !defined(PACKAGE)
18 #  define PACKAGE "example"  // for appcore_set_i18n()
19 #endif
20 
21 #if !defined(LOCALEDIR)
22 #  define LOCALEDIR "/opt/apps/com.slp.example/share/locale" // for appcore_set_i18n()
23 #endif
24 
25 #if !defined(EDJDIR)
26 #  define EDJDIR "/opt/apps/com.slp.example/share/edje"
27 #endif
28 
29 #define EDJ_FILE EDJDIR "/" PACKAGE ".edj"
30 #define GRP_MAIN "main"
31 
32 struct appdata
33 {
34         Evas_Object *win;
35         Evas_Object *ly_main;
36 
37         // add more variables here
38 };
39 
40 #endif // __APP_COMMON_H__ 
@endcode

Source example
@code
12 #include <stdio.h>
13 #include <appcore-efl.h>
14 #include <Ecore_X.h>
15 
16 #include "example.h"
17 
18 struct text_part {   // this for internationalization
19         char *part;
20         char *msgid;
21 };
22 
23 static struct text_part main_txt[] = {
24         { "txt_title", N_("Application template"), },
25         { "txt_mesg", N_("Click to exit"), },  // N_() is do nothing. Only for extracting the string
26 };
27
28
29 static void win_del(void *data, Evas_Object *obj, void *event)
30 {
31         elm_exit();
32 }
33 
34 static void main_quit_cb(void *data, Evas_Object *obj,
35                 const char *emission, const char *source)
36 {
37         elm_exit();
38 }
39 
40 static void update_ts(Evas_Object *eo, struct text_part *tp, int size)
41 {
42         int i;
43 
44         if (eo == NULL || tp == NULL || size < 0)
45                 return;
46 
47         for (i = 0; i < size; i++) {
48                 if (tp[i].part && tp[i].msgid)
49                         edje_object_part_text_set(eo,
50                                         tp[i].part, _(tp[i].msgid)); // _() return translated string
51         }
52 }
53 
54 static int lang_changed(void *data) // language changed callback
55 {
56         struct appdata *ad = data;
57 
58         if (ad->ly_main == NULL)
59                 return 0;
60 
61         update_ts(elm_layout_edje_get(ad->ly_main), main_txt,
62                         sizeof(main_txt)/sizeof(main_txt[0]));
63 
64         return 0;
65 }
66 
67 static int rotate(enum appcore_rm m, void *data) // rotation callback
68 {
69         struct appdata *ad = data;
70         int r;
71 
72         if (ad == NULL || ad->win == NULL)
73                 return 0;
74 
75         switch(m) {
76         case APPCORE_RM_PORTRAIT_NORMAL:
77                 r = 0;
78                 break;
79         case APPCORE_RM_PORTRAIT_REVERSE:
80                 r = 180;
81                 break;
82         case APPCORE_RM_LANDSCAPE_NORMAL:
83                 r = 270;
84                 break;
85         case APPCORE_RM_LANDSCAPE_REVERSE:
86                 r = 90;
87                 break;
88         default:
89                 r = -1;
90                 break;
91         }
92 
93         if (r >= 0) // Using this API, you can implement the rotation mode easily
94                 elm_win_rotation_with_resize_set(ad->win, r);
95 
96         return 0;
97 }
98 
99 static Evas_Object* create_win(const char *name)
100 {
101         Evas_Object *eo;
102         int w, h;
103 
104         eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
105         if (eo) {
106                 elm_win_title_set(eo, name);
107                 elm_win_borderless_set(eo, EINA_TRUE);
108                 evas_object_smart_callback_add(eo, "delete,request",
109                                 win_del, NULL);
110                 ecore_x_window_size_get(ecore_x_window_root_first_get(), // get root window(screen) size
111                                 &w, &h);
112                 evas_object_resize(eo, w, h);
113         }
114 
115         return eo;
116 }
117 
118 static Evas_Object* load_edj(Evas_Object *parent, const char *file,
119                 const char *group)
120 {
121         Evas_Object *eo;
122         int r;
123 
124         eo = elm_layout_add(parent);
125         if (eo) {
126                 r = elm_layout_file_set(eo, file, group);
127                 if (!r) {
128                         evas_object_del(eo);
129                         return NULL;
130                 }
131 
132                 evas_object_size_hint_weight_set(eo,
133                                 EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
134         }
135 
136         return eo;
137 }
138 
139 static int app_create(void *data)
140 {
141         struct appdata *ad = data;
142         Evas_Object *win;
143         Evas_Object *ly;
144         int r;
145 
146         // create window 
147         win = create_win(PACKAGE);
148         if (win == NULL)
149                 return -1;
150         ad->win = win;
151 
152         // load edje 
153         ly = load_edj(win, EDJ_FILE, GRP_MAIN);
154         if (ly == NULL)
155                 return -1;
156         elm_win_resize_object_add(win, ly); // This can make the EDJE object fitted in window size
157         edje_object_signal_callback_add(elm_layout_edje_get(ly),
158                         "EXIT", "*", main_quit_cb, NULL);
159         ad->ly_main = ly;
160         evas_object_show(ly);
161 
162         // init internationalization
163         r = appcore_set_i18n(PACKAGE, LOCALEDIR);
164         if (r)
165                 return -1;
166         lang_changed(ad); // call the language changed callback to update strings
167 
168         evas_object_show(win);
169 
170         // add system event callback 
171         appcore_set_event_callback(APPCORE_EVENT_LANG_CHANGE,
172                         lang_changed, ad);
173 
174         appcore_set_rotation_cb(rotate, ad); // set rotation callback
175 
176         // appcore measure time example 
177         printf("from AUL to %s(): %d msec\n", __func__,
178                         appcore_measure_time_from("APP_START_TIME"));
179 
180         appcore_measure_start();
181         return 0;
182 }
183 
184 static int app_terminate(void *data) // terminate callback
185 {
186         struct appdata *ad = data;
187 
188         if (ad->ly_main)
189                 evas_object_del(ad->ly_main);
190 
191         if (ad->win)
192                 evas_object_del(ad->win);
193 
194         return 0;
195 }
196 
197 static int app_pause(void *data) // pause callback
198 {
199         struct appdata *ad = data;
200 
201         return 0;
202 }
203 
204 static int app_resume(void *data) // resume callback
205 {
206         struct appdata *ad = data;
207 
208         return 0;
209 }
210 
211 static int app_reset(bundle *b, void *data) // reset callback
212 {
213         struct appdata *ad = data;
214 
215         // appcore measure time example 
216         printf("from AUL to %s(): %d msec\n", __func__,
217                         appcore_measure_time_from("APP_START_TIME"));
218         printf("from create to %s(): %d msec\n", __func__,
219                         appcore_measure_time());
220 
221         if (ad->win)
222                 elm_win_activate(ad->win); // You should make one of the window on top
223 
224         return 0;
225 }
226 
227 int main(int argc, char *argv[])
228 {
229         struct appdata ad;
230         struct appcore_ops ops = {  // fill the appcore_ops with callback functions
231                 .create = app_create,
232                 .terminate = app_terminate,
233                 .pause = app_pause,
234                 .resume = app_resume,
235                 .reset = app_reset,
236         };
237 
238         // appcore measure time example 
239         printf("from AUL to %s(): %d msec\n", __func__,
240                         appcore_measure_time_from("APP_START_TIME"));
241 
242         memset(&ad, 0x0, sizeof(struct appdata));
243         ops.data = &ad;
244 
245         return appcore_efl_main(PACKAGE, &argc, &argv, &ops); // start mainloop
246 }
@endcode

 @}
**/
