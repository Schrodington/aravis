#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

static char *arv_option_device_name = NULL;
static gboolean arv_option_list = FALSE;
static char *arv_option_debug_domains = NULL;

static const GOptionEntry arv_option_entries[] =
{
	{ "name",		'n', 0, G_OPTION_ARG_STRING,
		&arv_option_device_name,"Camera name", NULL},
	{ "list",		'l', 0, G_OPTION_ARG_NONE,
		&arv_option_list,	"List available features", NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	"Debug domains", NULL },
	{ NULL }
};

static void
arv_control_list_features (ArvGc *genicam, const char *feature, int level)
{
	ArvGcNode *node;

	node = arv_gc_get_node (genicam, feature);
	if (ARV_IS_GC_NODE (node)) {
		int i;

		for (i = 0; i < level; i++)
			printf ("    ");

		printf ("%s: '%s'\n", arv_gc_node_get_node_name (node), feature);

		if (ARV_IS_GC_CATEGORY (node)) {
			const GSList *features;
			const GSList *iter;

			features = arv_gc_category_get_features (ARV_GC_CATEGORY (node));

			for (iter = features; iter != NULL; iter = iter->next)
				arv_control_list_features (genicam, iter->data, level + 1);
		}
	}
}

int
main (int argc, char **argv)
{
	ArvDevice *device;
	GOptionContext *context;
	GError *error = NULL;
	int i;

	g_thread_init (NULL);
	g_type_init ();

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	arv_debug_enable (arv_option_debug_domains);

	device = arv_open_device (arv_option_device_name);
	if (!ARV_IS_DEVICE (device)) {
		printf ("Device '%s' not found\n", arv_option_device_name);
		return EXIT_FAILURE;
	}

	if (arv_option_list) {
		ArvGc *genicam;

		genicam = arv_device_get_genicam (device);
		arv_control_list_features (genicam, "Root",0);
	} else if (argc < 2) {
	} else
		for (i = 1; i < argc; i++) {
			ArvGcNode *feature;
			char **tokens;

			tokens = g_strsplit (argv[i], "=", 2);
			feature = arv_device_get_feature (device, tokens[0]);
			if (!ARV_IS_GC_NODE (feature))
				printf ("Feature '%s' not found\n", tokens[0]);
			else {
				if (tokens[1] != NULL)
					arv_gc_node_set_value_from_string (feature, tokens[1]);

				printf ("%s = %s\n", tokens[0], arv_gc_node_get_value_as_string (feature));
			}
			g_strfreev (tokens);
		}

	/* For debug purpose only */
	arv_shutdown ();

	return EXIT_SUCCESS;
}