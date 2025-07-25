#include <gtk/gtk.h>
#include <threads.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

static void runDialog(GtkWidget *dlg, GtkEntry *e) {
	static const char* filters[] = {"*.pdf", "*"};
	for (int i = 0; i < sizeof(filters)/sizeof(*filters); i++) {
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, filters[i]);
		gtk_file_filter_add_pattern(filter, filters[i]);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);
	}

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
		gchar *fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		gtk_entry_set_text(e, fn);
		g_free(fn);
	}

	gtk_widget_destroy (dlg);
}

static GtkWidget *entryIn;
static void bOpenClicked(GtkButton *button, GtkWidget *window) {
	GtkWidget *dlg = gtk_file_chooser_dialog_new(
		"Открыть файл", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN,
		"Отмена",  GTK_RESPONSE_CANCEL,
		"Открыть", GTK_RESPONSE_ACCEPT,
		NULL);

	runDialog(dlg, GTK_ENTRY(entryIn));
}

static GtkWidget *entryOut;
static void bSaveClicked(GtkButton *button, GtkWidget *window) {
	GtkWidget *dlg = gtk_file_chooser_dialog_new(
		"Сохранить файл", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE,
		"Отмена",  GTK_RESPONSE_CANCEL,
		"Сохранить", GTK_RESPONSE_ACCEPT,
		NULL);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

	runDialog(dlg, GTK_ENTRY(entryOut));
}


static volatile bool cont = false;
static GtkWidget *pb;
static int pulseThreadFunc(void *arg) {
	cont = true;
	while (cont) {
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pb));
		sleep(1);
	}

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb), 0);
}

static GtkWidget *combo;
static int runThreadFunc(void *window) {
	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "fork error\n");
		return 1;
	}

	if (0 == pid) {
		GValue value = G_VALUE_INIT;
		g_value_init(&value, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(combo), "active-id", &value);
		execlp("/usr/bin/gs", "/usr/bin/gs", "-sDEVICE=pdfwrite", "-dCompatibilityLevel=1.4",
			g_strconcat("-dPDFSETTINGS=/", g_value_get_string(&value), NULL),
			"-dNOPAUSE", "-dQUIET", "-dBATCH",
			g_strconcat("-sOutputFile=", gtk_entry_get_text(GTK_ENTRY(entryOut)), NULL),
			gtk_entry_get_text(GTK_ENTRY(entryIn)),
			NULL);
		exit(1);
	}

	int st;
	waitpid(pid, &st, 0);
	cont = false;
}

static void bRunClicked(GtkButton *button, GtkWidget *window) {
	thrd_t tp;
	thrd_create(&tp, pulseThreadFunc, NULL);
	thrd_t tr;
	thrd_create(&tr, runThreadFunc, window);
}

int main(int argc, char **argv) {
	gtk_init(&argc, &argv);

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(window), "PDF compress");
		gtk_window_set_default_size(GTK_WINDOW(window), 650, -1);
		g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

		GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
			gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

			gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Что будем сжимать?"), FALSE, FALSE, 0);

			GtkWidget *hboxIn = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
				entryIn = gtk_entry_new();
				gtk_box_pack_start(GTK_BOX(hboxIn), entryIn, TRUE, TRUE, 0);
				GtkWidget *bOpen = gtk_button_new_from_icon_name("document-open", GTK_ICON_SIZE_BUTTON);
					g_signal_connect (bOpen, "clicked", G_CALLBACK (bOpenClicked), window);
				gtk_box_pack_start(GTK_BOX(hboxIn), bOpen, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hboxIn, FALSE, FALSE, 0);

			gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(""), FALSE, FALSE, 0);

			gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Куда будем сохранять?"), FALSE, FALSE, 0);

			GtkWidget *hboxOut = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
				entryOut = gtk_entry_new();
				gtk_box_pack_start(GTK_BOX(hboxOut), entryOut, TRUE, TRUE, 0);
				GtkWidget *bSave = gtk_button_new_from_icon_name("document-save-as", GTK_ICON_SIZE_BUTTON);
					g_signal_connect (bSave, "clicked", G_CALLBACK (bSaveClicked), window);
				gtk_box_pack_start(GTK_BOX(hboxOut), bSave, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hboxOut, FALSE, FALSE, 0);

			gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(""), FALSE, FALSE, 0);

			combo = gtk_combo_box_text_new ();
				gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), "screen", "Низкое качество - 72 dpi)");
				gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), "ebook", "Среднее качество - 150 dpi");
				gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), "prepress", "Высокое качество - 300 dpi)");
				gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
			gtk_container_add (GTK_CONTAINER (vbox), combo);

			gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(""), FALSE, FALSE, 0);

			GtkWidget *hboxRun = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
				GtkWidget *bRun = gtk_button_new_with_label("Запуск");
					g_signal_connect (bRun, "clicked", G_CALLBACK (bRunClicked), window);
				gtk_box_pack_start(GTK_BOX(hboxRun), bRun, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hboxRun, FALSE, FALSE, 0);

			gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(""), FALSE, FALSE, 0);

			pb = gtk_progress_bar_new();
			gtk_box_pack_start(GTK_BOX(vbox), pb, FALSE, FALSE, 0);
		gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show_all(window);

	gtk_main();
	return 0;
}
