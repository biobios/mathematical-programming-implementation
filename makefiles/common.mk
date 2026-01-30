# エスケープシーケンス定義
C_RESET   := \033[0m
C_BOLD    := \033[1m
C_GREY    := \033[90m
C_RED     := \033[31m
C_GREEN   := \033[32m
C_YELLOW  := \033[33m
C_MAGENTA := \033[35m
C_CYAN    := \033[36m

# ファイル名の表示幅（これを超えるとレイアウトが崩れる）
FILE_WIDTH := 35

# ---------------------------------------------------------
# ログ出力関数定義
# 引数 $1: プロジェクト名 (e.g., eax_tabu, libmpilib)
# 引数 $2: 対象ファイル名
# ---------------------------------------------------------

# 1. コンパイル (CXX / CC) - 緑
define log_cxx
	@printf "  $(C_GREEN)CXX$(C_RESET)   %-$(FILE_WIDTH)s $(C_GREY)[%s]$(C_RESET)\n" "$2" "$1"
endef

# 2. リンク (LD) - 黄 (太字で強調)
define log_ld
	@printf "  $(C_YELLOW)LD $(C_RESET)   $(C_BOLD)%-$(FILE_WIDTH)s$(C_RESET) $(C_GREY)[%s]$(C_RESET)\n" "$2" "$1"
endef

# 3. アーカイブ/静的ライブラリ (AR) - マゼンタ
define log_ar
	@printf "  $(C_MAGENTA)AR $(C_RESET)   $(C_BOLD)%-$(FILE_WIDTH)s$(C_RESET) $(C_GREY)[%s]$(C_RESET)\n" "$2" "$1"
endef

# 4. プログラム実行 (RUN) - シアン
# 引数 $2 には実行ファイル名、あるいは "Running tests..." 等のメッセージも可
define log_exec
	@printf "  $(C_CYAN)RUN$(C_RESET)   %-$(FILE_WIDTH)s $(C_GREY)[%s]$(C_RESET)\n" "$2" "$1"
endef

# 5. クリーン (RM) - 赤
define log_rm
	@printf "  $(C_RED)RM $(C_RESET)   %-$(FILE_WIDTH)s $(C_GREY)[%s]$(C_RESET)\n" "$2" "$1"
endef