SHELL = /bin/sh

# Directories are intentionally not managed by variables for better 
SOURCE						:= src
BUILD							:= docs
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
PAGE_TPL					:= page.html
INDEX_TPL					:= writing.html

# Pandoc related stuff
PANDOC_VERSION		:= 3.1.9
PANDOC						:= pandoc 
PANDOC_SHARED_OPT	:= -f gfm \
										 -t markdown-smart \
										 --standalone \
										 --to html5 \
										 --highlight-style tango \
										 --from=markdown+yaml_metadata_block
PANDOC_BEFORE_INDEX := --include-before-body header.html
PANDOC_HTML_OPT		:= -M \
										 --document-css=false \
										 --css css/style.css
PANDOC_PAGE_TPL		:= --template $(TPL)/$(PAGE_TPL)
PANDOC_INDEX_TPL	:= --template $(TPL)/$(INDEX_TPL)
PANDOC_METADATA		:= --metadata title-author="Max"

.PHONY: all
all: $(BUILD) $(TARGET_DIRS) $(TARGET_CSS) $(TARGET_DOCS) $(BUILD)/writing.html

# In case the Makefile itself changes
all: .EXTRA_PREREQS := $(abspath $(lastword $(MAKEFILE_LIST)))

# Create directory to hold CSS and HTML files
$(BUILD):
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
$(BUILD)/%.html: $(SOURCE)/%.md header.html $(TPL)/$(PAGE_TPL) | $(BUILD)
	@printf "Converting $(notdir $<) >>> $(notdir $@)\n"
	@$(PANDOC) \
		$(PANDOC_SHARED_OPT) \
		$(PANDOC_PAGE_TPL) \
		$(PANDOC_HTML_OPT) \
		$(PANDOC_BEFORE_INDEX) \
		$(PANDOC_METADATA) \
		--variable="modified-date:$$(date '+%Y-%m-%d')" \
		$< -o $@

# Source metadata from all files
.INTERMEDIATE: index.yaml
index.yaml: index.sh $(TPL)/$(INDEX_TPL) $(SOURCE_DOCS)
	@echo 'Parsing metadata...'
	@./index.sh

# Create writing.html
$(BUILD)/writing.html: index.yaml
	@echo 'Building writing.html...'
	@$(PANDOC) \
		$(PANDOC_SHARED_OPT) \
		--metadata-file index.yaml \
		$(PANDOC_INDEX_TPL) \
    $(PANDOC_HTML_OPT) \
		$(PANDOC_BEFORE_INDEX) \
		$(PANDOC_METADATA) \
		-o $(BUILD)/writing.html /dev/null

# Make sure we rebuild the index when source files change
$(BUILD)/writing.html: $(patsubst $(SRC)/%.md,$(BUILD)/%.html,$(wildcard $(SRC)/*.md))

# Deploy
.PHONY: deploy
deploy: clean all

# Clean the build directory
.PHONY: clean
clean:
	rm -rf $(BUILD)
