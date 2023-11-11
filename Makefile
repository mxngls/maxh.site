SHELL = /bin/sh

# Directories are intentionally not managed by variables for better 
SOURCE						:= src
BUILD							:= html
TPL								:= templates

# Source and target files
SOURCE_DOCS				:= $(wildcard $(SOURCE)/*.md)
SOURCE_CSS				:= $(wildcard style/*.css)

EXPORTED_DOCS			:= $(addprefix $(BUILD)/,$(notdir $(SOURCE_DOCS:.md=.html)))
EXPORTED_CSS			:= $(addprefix $(BUILD)/css/,$(notdir $(SOURCE_CSS)))

# Pandoc related stuff
PANDOC_VERSION		:= 3.1.8
PANDOC						:= pandoc 
PANDOC_SHARED_OPT	:= -f gfm \
										 -t markdown-smart \
										 --standalone \
										 --to html5 \
										 --highlight-style tango \
										 --from=markdown+yaml_metadata_block
PANDOC_HTML_OPT		:= --include-before-body header.html \
										 -M \
										 --document-css=false \
										 --css ../style/style.css
PANDOC_PAGE_TPL		:= --template $(TPL)/page.tpl 
PANDOC_INDEX_TPL	:= --template $(TPL)/index.tpl 
PANDOC_METADATA		:= --metadata title-author="Max"

# Convert to upper case
uppercase = $(shell echo '$*' | perl -nE 'say ucfirst')

.PHONY: all
all: $(BUILD) $(EXPORTED_CSS) $(EXPORTED_DOCS) $(BUILD)/index.html

# In case the Makefile itself changes
all: .EXTRA_PREREQS := $(abspath $(lastword $(MAKEFILE_LIST)))

html:
	mkdir -p $(BUILD)/css

$(EXPORTED_DOCS): $(SOURCE_DOCS) header.html | $(BUILD)
	@printf "$< >>> $@\n"
	@$(PANDOC) \
		$(PANDOC_SHARED_OPT) \
		$(PANDOC_PAGE_TPL) \
		$(PANDOC_HTML_OPT) \
		$(PANDOC_METADATA) \
		--variable="modified-date:$$(date '+%Y-%m-%d')" \
		$< -o $@ > /dev/null 2>&1

index.yaml: 
	@./index.sh

$(BUILD)/index.html: $(SRC_DOCS) templates/index.tpl index.sh index.yaml | index.yaml
	echo 'Creating index.html'
	@$(PANDOC) \
		$(PANDOC_SHARED_OPT) \
		--metadata-file index.yaml \
		$(PANDOC_INDEX_TPL) \
		$(PANDOC_HTML_OPT) \
		$(PANDOC_METADATA) \
		-o html/index.html /dev/null

$(BUILD)/index.html: $(patsubst $(SRC)/%.md,$(BUILD)/%.html,$(wildcard $(SRC)/*.md))

$(BUILD)/css/%.css: style/%.css
	cp $< $@

.PHONY: clean
clean:
	rm -rf html
