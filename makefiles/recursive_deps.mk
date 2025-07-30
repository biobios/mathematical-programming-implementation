# 幅優先探索

ifndef ALREADY_INCLUDED_LIBS
ALREADY_INCLUDED_LIBS :=
INCLUDING_LIBS := $(DEPEND_LIBS)
endif


ifneq ($(INCLUDING_LIBS),)
# INCLUDING_LIBS が空でない場合、INCLUDING_LIBS の先頭を取り出して include する
-include $(patsubst %, $(ROOT_DIR)/src/lib%/dependencies.mk, $(firstword $(INCLUDING_LIBS)))
# ここで取り出したライブラリを ALREADY_INCLUDED_LIBS に追加し、INCLUDING_LIBS からは除外する
ALREADY_INCLUDED_LIBS += $(firstword $(INCLUDING_LIBS))
INCLUDING_LIBS := $(filter-out $(firstword $(INCLUDING_LIBS)), $(INCLUDING_LIBS))
# include したファイルの DEPEND_LIBS のうち、 ALREADY_INCLUDED_LIBS と INCLUDING_LIBS の両方に含まれないものを INCLUDING_LIBS に追加する
INCLUDING_LIBS += $(filter-out $(INCLUDING_LIBS), $(filter-out $(ALREADY_INCLUDED_LIBS), $(DEPEND_LIBS)))
# 再帰的にこの Makefile を呼び出す
-include $(ROOT_DIR)/makefiles/recursive_deps.mk
else
# INCLUDING_LIBS が空の場合、再帰を終了
# ALREADY_INCLUDED_LIBS を最終的な DEPEND_LIBS として設定
DEPEND_LIBS := $(ALREADY_INCLUDED_LIBS)
endif