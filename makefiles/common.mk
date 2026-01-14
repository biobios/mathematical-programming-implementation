INDENT_STR = $(shell printf '%*s' $$(($(MAKELEVEL) * 2)) '')

define log
	@echo "$(INDENT_STR)$1"
endef