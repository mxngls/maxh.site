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
		const contentContainer = document.querySelector("main");
		if (!contentContainer) {
			throw new error(
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
			throw new error(`${SITE_FOOTNOTE_TAGNAME}: Footnote list not found`);
		}

		ol.appendChild(li);

		const contentContainer = document.querySelector("main");
		contentContainer.appendChild(this.footnoteContainer);
	}

	initContainer(contentContainer, footnoteContainer) {
		if (!footnoteContainer) {
			footnoteContainer = document.createElement("div");
			footnoteContainer.id = "footnotes";
			footnoteContainer.appendChild(document.createElement("ol"));
			contentContainer.appendChild(footnoteContainer);
		}
		return footnoteContainer;
	}
}

class SiteMenu extends HTMLElement {
	static level = 0;

	constructor() {
		super();

		const parentMenu = this.parentElement?.closest(SITE_MENU_TAGNAME);

		this.isTop = this.id === "top-menu";
		this.level = parentMenu ? parentMenu.level + 1 : 1;
		this.content = Array.from(this.children).map((child, index) => {
			const nudge = 0.6;
			const tiltRight =
				Math.random() < index % 2 === 0 ? 0.5 + nudge * 0.5 : 0.5 - nudge * 0.5;

			const rotation = ((tiltRight ? 1 : -1) * (Math.random() * 20 + index * 0.4)).toFixed(4);

			const delay = index * 0.01;
			const style = `--delay: ${delay}s; --rotation: ${rotation}deg`;

			if (child.tagName !== SITE_MENU_TAGNAME.toUpperCase()) {
				child.classList.add("site-menu-item");
			}

			const li = document.createElement("li");
			li.style = style;
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
			this.innerHTML = `<div class="wrapper hidden">
		    <menu>
				${this.content.map((li) => li.outerHTML).join("")}
		    </menu>
		</div>
    	<button class="site-menu-item">${label}</button>
    	<div class="background"></div>`;
		} else {
			this.innerHTML = `<div class="wrapper hidden">
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
		const button = this.querySelector(":scope > button");
		button.addEventListener("click", () => this.toggleSelf());

		if (this.isTop) {
			const background = this.querySelector("#top-menu .background");
			background.addEventListener("click", () => this.toggleBackground());

			this.disableBackButtonIfAtRoot();
		}
	}

	disableBackButtonIfAtRoot() {
		const isAtRoot = window.location.pathname === "/";
		const backButton = Array.from(this.querySelectorAll("button.site-menu-item")).find((btn) =>
			btn.textContent.includes("← Back"),
		);

		if (backButton && isAtRoot) {
			backButton.disabled = true;
		}
	}

	toggleSelf() {
		const wrapper = this.querySelector(":scope > .wrapper");

		if (this.isTop) {
			this.querySelectorAll(`:scope ${SITE_MENU_TAGNAME} .wrapper`).forEach((w) =>
				w.classList.add("hidden"),
			);
			wrapper.classList.toggle("hidden");
			document.querySelector(".background").classList.toggle("active");
		} else {
			document
				.getElementById("top-menu")
				.querySelectorAll(`:scope ${SITE_MENU_TAGNAME} .wrapper`)
				.forEach((w) => {
					if (wrapper.isSameNode(w)) {
						w.classList.toggle("hidden");
					} else if (!w.parentElement.contains(this)) {
						w.classList.add("hidden");
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

		// Find all open nested menus (not top-level)
		const openMenus = document.querySelectorAll(
			`${SITE_MENU_TAGNAME}:not([data-level="1"]) > .wrapper:not(.hidden)`,
		);

		// Mark menus as deepest if they have no open child menus
		openMenus.forEach((wrapper) => {
			const menu = wrapper.parentElement;
			const hasOpenChild = wrapper.querySelector(".wrapper:not(.hidden)");

			if (!hasOpenChild) {
				menu.classList.add("deepest");
			}
		});
	}

	toggleBackground() {
		document
			.getElementById("top-menu")
			.querySelectorAll(`:scope ${SITE_MENU_TAGNAME} .wrapper`)
			.forEach((w) => w.classList.add("hidden"));

		this.querySelector(":scope > .wrapper").classList.add("hidden");
		document.querySelector(".background").classList.remove("active");

		this.updateDeepestMenu();
	}
}

customElements.define(SITE_FOOTNOTE_TAGNAME, SiteFootnote);
customElements.define(SITE_MENU_TAGNAME, SiteMenu);
