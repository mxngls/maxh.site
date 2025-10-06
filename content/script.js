class Footnote extends HTMLElement {
	static footnoteCounter = 0;

	constructor() {
		super();

		this.footnoteNumber = ++Footnote.footnoteCounter;
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
			console.error("FootnoteRef: Content container to append footnotes to not found");
			return;
		}

		const footnoteContainer = document.getElementById("footnotes");
		if (!footnoteContainer || !this.footnoteContainer) {
			this.footnoteContainer = this.initContainer(contentContainer, footnoteContainer);
		}
		const content = this.innerHTML.trim();

		if (!content) {
			console.warn("FootnoteRef: Empty footnote content, skipping");
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
			console.error("FootnoteRef: Footnote list not found");
			return;
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

customElements.define("x-footnote", Footnote);
