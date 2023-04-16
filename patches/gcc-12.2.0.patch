diff --git a/gcc/cgraph.cc b/gcc/cgraph.cc
index 4bb9e7ba6..540d7268c 100644
--- a/gcc/cgraph.cc
+++ b/gcc/cgraph.cc
@@ -2155,6 +2155,8 @@ cgraph_node::dump (FILE *f)
     }
   if (tp_first_run > 0)
     fprintf (f, " first_run:%" PRId64, (int64_t) tp_first_run);
+  if (text_sorted_order > 0)
+    fprintf (f, " text_sorted_order:%i", text_sorted_order);
   if (cgraph_node *origin = nested_function_origin (this))
     fprintf (f, " nested in:%s", origin->dump_asm_name ());
   if (gimple_has_body_p (decl))
diff --git a/gcc/cgraph.h b/gcc/cgraph.h
index 8c512b648..5ea0c3d29 100644
--- a/gcc/cgraph.h
+++ b/gcc/cgraph.h
@@ -1426,6 +1426,8 @@ struct GTY((tag ("SYMTAB_FUNCTION"))) cgraph_node : public symtab_node
   int unit_id;
   /* Time profiler: first run of function.  */
   int tp_first_run;
+  /* Order in .text.sorted.* section.  */
+  int text_sorted_order;
 
   /* True when symbol is a thunk.  */
   unsigned thunk : 1;
diff --git a/gcc/cgraphclones.cc b/gcc/cgraphclones.cc
index eb0fa87b5..d34458cb4 100644
--- a/gcc/cgraphclones.cc
+++ b/gcc/cgraphclones.cc
@@ -403,6 +403,7 @@ cgraph_node::create_clone (tree new_decl, profile_count prof_count,
   new_node->rtl = rtl;
   new_node->frequency = frequency;
   new_node->tp_first_run = tp_first_run;
+  new_node->text_sorted_order = text_sorted_order;
   new_node->tm_clone = tm_clone;
   new_node->icf_merged = icf_merged;
   new_node->thunk = thunk;
diff --git a/gcc/lto-cgraph.cc b/gcc/lto-cgraph.cc
index 237743ef0..80881ec69 100644
--- a/gcc/lto-cgraph.cc
+++ b/gcc/lto-cgraph.cc
@@ -503,6 +503,7 @@ lto_output_node (struct lto_simple_output_block *ob, struct cgraph_node *node,
     section = "";
 
   streamer_write_hwi_stream (ob->main_stream, node->tp_first_run);
+  streamer_write_hwi_stream (ob->main_stream, node->text_sorted_order);
 
   bp = bitpack_create (ob->main_stream);
   bp_pack_value (&bp, node->local, 1);
@@ -1302,6 +1303,7 @@ input_node (struct lto_file_decl_data *file_data,
 		    "node with uid %d", node->get_uid ());
 
   node->tp_first_run = streamer_read_uhwi (ib);
+  node->text_sorted_order = streamer_read_uhwi (ib);
 
   bp = streamer_read_bitpack (ib);
 
diff --git a/gcc/varasm.cc b/gcc/varasm.cc
index 021e912a3..56607a43d 100644
--- a/gcc/varasm.cc
+++ b/gcc/varasm.cc
@@ -629,6 +629,14 @@ default_function_section (tree decl, enum node_frequency freq,
   if (exit && freq != NODE_FREQUENCY_UNLIKELY_EXECUTED)
     return get_named_text_section (decl, ".text.exit", NULL);
 
+  cgraph_node *node = cgraph_node::get (decl);
+  if (node->text_sorted_order > 0 && freq != NODE_FREQUENCY_UNLIKELY_EXECUTED)
+    {
+      char section_name[64];
+      sprintf (section_name, ".text.sorted.%010d", node->text_sorted_order);
+      return get_named_text_section (decl, section_name, NULL);
+    }
+
   /* Group cold functions together, similarly for hot code.  */
   switch (freq)
     {
