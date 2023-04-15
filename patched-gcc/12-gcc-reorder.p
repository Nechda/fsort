diff --git a/freorder-ipa.cpp b/freorder-ipa.cpp
index 3cc38be..105eba7 100644
--- a/freorder-ipa.cpp
+++ b/freorder-ipa.cpp
@@ -69,7 +69,7 @@ static unsigned int reorder_functions() {
 
     cgraph_node *node;
     FOR_EACH_DEFINED_FUNCTION(node) {
-        if (node == nullptr && !node->alias && !node->global.inlined_to)
+        if (node == nullptr && !node->alias && !node->inlined_to)
             continue;
 
         int id = 0;
