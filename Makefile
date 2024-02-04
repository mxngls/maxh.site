SHELL = /bin/sh

# Directories are intentionally not managed by variables for better 
SOURCE						:= src
BUILD							:= docs
TPL								:= templates
ASSETS						:= assets

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
SOURCE_ASSETS			:= $(wildcard $(ASSETS)/*)

TARGET_DIRS				:= $(subst $(SOURCE),$(BUILD),$(SOURCE_DIRS))
TARGET_DOCS				:= $(patsubst $(SOURCE)/%,$(BUILD)/%,$(SOURCE_DOCS:.md=.html))
TARGET_CSS				:= $(addprefix $(BUILD)/css/,$(notdir $(SOURCE_CSS)))
TARGET_ASSETS			:= $(addprefix $(BUILD)/, $(SOURCE_ASSETS))
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
all: $(BUILD) $(TARGET_DIRS) $(TARGET_CSS) $(TARGET_ASSETS) $(TARGET_DOCS) $(BUILD)/writing.html

# Create directory to hold CSS and HTML files
$(BUILD):
	@echo 'Creating directory for css files and other assets...'
	mkdir -p $(BUILD)/css
	mkdir -p $(BUILD)/$(ASSETS)

# Copy CSS files into the build directory
$(TARGET_CSS): *.css
	cp $< $@

# Copy other assets into the build directory
$(TARGET_ASSETS): $(SOURCE_ASSETS)
	cp $< $@

$(TARGET_DIRS): $(SOURCE_DIRS)
	@echo 'Creating directies for source files...'
	mkdir -p $@

# Convert Markdown to HTML
$(TARGET_DOCS): $(SOURCE_DOCS) header.html $(TPL)/$(PAGE_TPL)
	@printf "Converting $(notdir $<) >>> $(notdir $@)\n"
	@$(PANDOC) \
		$(PANDOC_SHARED_OPT) \
		$(PANDOC_PAGE_TPL) \
		$(PANDOC_HTML_OPT) \
		$(PANDOC_BEFORE_INDEX) \
		$(PANDOC_METADATA) \
		--variable="date:$$(grep -h -w -m 1 'date:' $< | \
			sed -e 's/date:[[:space:]]*//g' | \
			tr -d \" | \
			{ read DATE; date -j -f '%Y/%m/%d' +'%a, %-e %B %Y' $$DATE; } )" \
		--variable="modified-date:$$(git log \
			-1 \
			--date='format:%a, %e %B %G' \
      --format='%cd' \
			$< | \
			sed -e 's/-/\//g')" \
		$< -o $@

# Source metadata from all files
.INTERMEDIATE: index.yaml
index.yaml: index.sh $(TPL)/$(INDEX_TPL) $(SOURCE_DOCS) header.html
	@echo 'Parsing metadata...'
	@./index.sh

# Create writing.html
$(BUILD)/writing.html: index.yaml $(SOURCE_DOCS)
	@echo 'Building writing.html...'
	@$(PANDOC) \
		$(PANDOC_SHARED_OPT) \
		--metadata-file index.yaml \
		$(PANDOC_INDEX_TPL) \
    $(PANDOC_HTML_OPT) \
		$(PANDOC_BEFORE_INDEX) \
		$(PANDOC_METADATA) \
		-o $(BUILD)/writing.html /dev/null

# Deploy
.PHONY: deploy
deploy: clean all

# Clean the build directory
.PHONY: clean
clean:
	rm -rf $(BUILD)
