const SITE_FOOTNOTE_TAGNAME = "site-footnote";
const SITE_MENU_TAGNAME = "site-menu";

class SiteFootnote extends HTMLElement {
	static footnoteCounter = 0;

	constructor() {
		super();

		this.footnoteNumber = ++SiteFootnote.footnoteCounter;
	}

	connectedCallback() {
		if (this.innerHTML.trim()) {
			this.processContent();
		} else {
			// Watch for content changes
			const observer = new MutationObserver(() => {
				if (this.innerHTML.trim()) {
					observer.disconnect();
					this.processContent();
				}
			});
			observer.observe(this, { childList: true, subtree: true });
		}
	}

	processContent() {
		const contentContainer = document.getElementById("post-main");
		if (!contentContainer) {
			throw new Error(
				`${SITE_FOOTNOTE_TAGNAME}: Content container to append footnotes to not found`,
			);
		}

		const footnoteContainer = document.getElementById("footnotes");
		if (!footnoteContainer || !this.footnoteContainer) {
			this.footnoteContainer = this.initContainer(contentContainer, footnoteContainer);
		}
		const content = this.innerHTML.trim();

		if (!content) {
			console.warn(`${SITE_FOOTNOTE_TAGNAME}: Empty footnote content, skipping`);
			this.remove();
			return;
		}

		this.addFootnote(content);
		this.addFootnoteRef();
	}

	addFootnoteRef() {
		const a = document.createElement("a");

		a.classList.add("footnote-ref");
		a.href = `#footnote-${this.footnoteNumber}`;
		a.id = `footnote-ref-${this.footnoteNumber}`;
		a.innerText = this.footnoteNumber;

		const sup = document.createElement("sup");

		sup.appendChild(a);

		this.replaceWith(sup);
	}

	addFootnote(content) {
		const li = document.createElement("li");
		const p = document.createElement("p");
		const a = document.createElement("a");

		li.id = `footnote-${this.footnoteNumber}`;
		p.innerHTML = content;
		a.classList.add("footnote-back");
		a.href = `#footnote-ref-${this.footnoteNumber}`;
		a.innerText = "↩";

		li.appendChild(p);
		p.appendChild(a);

		const ol = this.footnoteContainer.getElementsByTagName("ol").item(0);
		if (!ol) {
			throw new Error(`${SITE_FOOTNOTE_TAGNAME}: Footnote list not found`);
		}

		ol.appendChild(li);
	}

	initContainer(contentContainer, footnoteContainer) {
		if (!footnoteContainer) {
			footnoteContainer = document.createElement("div");
			footnoteContainer.id = "footnotes";
			footnoteContainer.appendChild(document.createElement("ol"));

			const dateUpdated = document.getElementById("post-date");
			if (dateUpdated) {
				// Insert footnotes before the date element
				contentContainer.insertBefore(footnoteContainer, dateUpdated);
			} else {
				// No date element, append to end
				contentContainer.appendChild(footnoteContainer);
			}
		}

		return footnoteContainer;
	}
}

class SiteMenu extends HTMLElement {
	static level = 0;

	constructor() {
		super();

		const parentMenu = this.parentElement?.closest(SITE_MENU_TAGNAME);

		this.isTop = this.id === "site-menu-top";
		this.dataset.enabled = true;
		this.level = parentMenu ? parentMenu.level + 1 : 1;
		this.content = Array.from(this.children).map((child) => {
			if (child.tagName !== SITE_MENU_TAGNAME.toUpperCase()) {
				child.classList.add("site-menu-item");
			}

			const li = document.createElement("li");
			li.appendChild(child);

			return li;
		});
	}
	connectedCallback() {
		const label = this.getAttribute("label");

		if (!label) {
			throw new Error(`${SITE_MENU_TAGNAME}: "label" attribute is required`);
		}

		if (this.level > 3) {
			console.warn(`${SITE_MENU_TAGNAME}: Exceeding max sub-menu nesting level of 3`);
			this.remove();
			return;
		}

		this.dataset.level = this.level;
		this.style.setProperty("--level", this.level);

		if (this.isTop) {
			this.innerHTML = `<div class="wrapper">
		    <menu>
				${this.content.map((li) => li.outerHTML).join("")}
		    </menu>
		</div>
		<div class="site-menu-control-wrap">
			<button
				id="site-menu-control-back-btn"
				class="site-menu-control"
				onclick="window.location='/'"
			>Back</button>
			<button
				id="site-menu-control-top-btn"
				class="site-menu-control"
			>Top</button>
			<button id="site-menu-main-toggle" class="site-menu-control">${label}</button>
		</div>
		`;
		} else {
			this.innerHTML = `<div class="wrapper">
		    <ul>
				${this.content.map((li) => li.outerHTML).join("")}
		    </ul>
		</div>
    	<button class="site-menu-item">${label}</button>`;
		}

		this.setupEventListeners();
	}

	setupEventListeners() {
		// `:scope` necessary to include the node which is currently toggled
		const button = this.isTop
			? this.querySelector(":scope > .site-menu-control-wrap > button#site-menu-main-toggle")
			: this.querySelector(":scope > button");
		button.addEventListener("click", () => this.toggleSelf());

		if (this.isTop) {
			const background = document.getElementById("background");
			background.addEventListener("click", () => this.toggleBackground());

			// Add hover effect for background
			background.addEventListener("mouseenter", () => {
				this.classList.add("hover");
			});
			background.addEventListener("mouseleave", () => {
				this.classList.remove("hover");
			});

			const topButton = document.getElementById("site-menu-control-top-btn");
			if (topButton) {
				topButton.addEventListener("click", () => {
					background.classList.remove("active");
					document.getElementById("site-menu-top").classList.remove("open");

					window.scrollTo({ top: 0, behavior: "smooth" });
				});
			}

			this.disableBackButtonIfAtRoot();
		}
	}

	disableBackButtonIfAtRoot() {
		const isAtRoot = window.location.pathname === "/";
		const backButton = document.getElementById("site-menu-control-back-btn");

		if (backButton && isAtRoot) {
			backButton.disabled = true;
		}
	}

	toggleSelf() {
		if (this.isTop) {
			// Close all nested menus
			this.querySelectorAll(`:scope ${SITE_MENU_TAGNAME}`).forEach((menu) => {
				menu.classList.remove("open");
			});

			// Toggle this menu
			this.classList.toggle("open");
			document.getElementById("background").classList.toggle("active");
		} else {
			// Check if we're closing this menu
			const isClosing = this.classList.contains("open");

			// Handle nested menu toggles
			document
				.getElementById("site-menu-top")
				.querySelectorAll(`:scope ${SITE_MENU_TAGNAME}`)
				.forEach((menu) => {
					if (menu === this) {
						menu.classList.toggle("open");
					} else if (isClosing && this.contains(menu)) {
						// If closing this menu, close all child menus
						menu.classList.remove("open");
					} else if (!this.contains(menu) && !menu.contains(this)) {
						menu.classList.remove("open");
					}
				});
		}

		this.updateDeepestMenu();
	}

	updateDeepestMenu() {
		// Remove 'deepest' from all menus
		document.querySelectorAll(`${SITE_MENU_TAGNAME}.deepest`).forEach((menu) => {
			menu.classList.remove("deepest");
		});

		// Find all open menus
		const openMenus = document.querySelectorAll(`${SITE_MENU_TAGNAME}.open`);

		// Mark menus as deepest if they have no open child menus
		openMenus.forEach((menu) => {
			const hasOpenChild = menu.querySelector(`${SITE_MENU_TAGNAME}.open`);

			if (!hasOpenChild) {
				menu.classList.add("deepest");
			}
		});
	}

	toggleBackground() {
		// Close all menus including the top menu
		const topMenu = document.getElementById("site-menu-top");
		topMenu.classList.remove("open");

		topMenu.querySelectorAll(`:scope ${SITE_MENU_TAGNAME}`).forEach((menu) => {
			menu.classList.remove("open");
		});

		document.getElementById("background").classList.remove("active");

		this.updateDeepestMenu();
	}
}

customElements.define(SITE_FOOTNOTE_TAGNAME, SiteFootnote);
customElements.define(SITE_MENU_TAGNAME, SiteMenu);
