SHELL = /bin/sh

# Directories are intentionally not managed by variables for better 
SOURCE						:= src
BUILD							:= html
TPL								:= templates

# Source and target files
SOURCE_DIRS				:= $(shell find $(SOURCE) \
										 -type d \
										 -mindepth 1 \
										 -not -path '$(SOURCE)/drafts'\
										 )
SOURCE_DOCS				:= $(shell find $(SOURCE) \
										 -type f \
										 -name '*.md' \
										 -not -path '$(SOURCE)/drafts/*'\
										 )
SOURCE_CSS				:= $(wildcard *.css)

TARGET_DIRS				:= $(subst $(SOURCE),$(BUILD),$(SOURCE_DIRS))
TARGET_DOCS				:= $(patsubst $(SOURCE)/%,$(BUILD)/%,$(SOURCE_DOCS:.md=.html))
TARGET_CSS				:= $(addprefix $(BUILD)/css/,$(notdir $(SOURCE_CSS)))

# Pandoc related stuff
PANDOC_VERSION		:= 3.1.9
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
										 --css css/style.css
PANDOC_PAGE_TPL		:= --template $(TPL)/page.tpl 
PANDOC_INDEX_TPL	:= --template $(TPL)/index.tpl 
PANDOC_METADATA		:= --metadata title-author="Max"

.PHONY: all
all: clean $(BUILD) $(TARGET_DIRS) $(TARGET_CSS) $(TARGET_DOCS) $(BUILD)/index.html

# In case the Makefile itself changes
all: .EXTRA_PREREQS := $(abspath $(lastword $(MAKEFILE_LIST)))

# Create directory to hold CSS and HTML files
html:
	@echo 'Creating directory for css files...'
	mkdir -p $(BUILD)/css

# Copy CSS files into the build directory
$(BUILD)/css/%.css: %.css
	@echo 'Copying css files...'
	cp $< $@

$(TARGET_DIRS): $(SOURCE_DIRS)
	@echo 'Creating directies for source files...'
	mkdir -p $@

# Convert Markdown to HTML
$(BUILD)/%.html: $(SOURCE)/%.md header.html | $(BUILD)
	@printf "Converting $(notdir $<) >>> $(notdir $@)\n"
	@$(PANDOC) \
		$(PANDOC_SHARED_OPT) \
		$(PANDOC_PAGE_TPL) \
		$(PANDOC_HTML_OPT) \
		$(PANDOC_METADATA) \
		--variable="modified-date:$$(date '+%Y-%m-%d')" \
		$< -o $@

# Source metadata from all files
.INTERMEDIATE: index.yaml
index.yaml: index.sh $(TPL)/index.tpl $(SOURCE_DOCS)
	@echo 'Parsing metadata...'
	@./index.sh

# Create index.html
$(BUILD)/index.html: index.yaml
	@echo 'Building index.html...'
	@$(PANDOC) \
		$(PANDOC_SHARED_OPT) \
		--metadata-file index.yaml \
		$(PANDOC_INDEX_TPL) \
		$(PANDOC_HTML_OPT) \
		$(PANDOC_METADATA) \
		-o html/index.html /dev/null

# Make sure we rebuild the index when source files change
$(BUILD)/index.html: $(patsubst $(SRC)/%.md,$(BUILD)/%.html,$(wildcard $(SRC)/*.md))

# Clean the build directory
.PHONY: clean
clean:
	rm -rf html
