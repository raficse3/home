// ========================================
// SCROLL REVEAL
// ========================================

const revealElements = document.querySelectorAll(".reveal");

const revealObserver = new IntersectionObserver(
    (entries) => {
        entries.forEach((entry) => {

            if (entry.isIntersecting) {
                entry.target.classList.add("active");
                revealObserver.unobserve(entry.target);

            }

        });
    },
    {
        threshold: 0.12
    }
);

revealElements.forEach((element) => {
    revealObserver.observe(element);
});


// ========================================
// MOBILE MENU
// ========================================

const menuBtn = document.getElementById("menuBtn");
const navLinks = document.querySelector(".nav-links");

menuBtn.addEventListener("click", () => {

    navLinks.classList.toggle("mobile-active");

});


// Close menu after clicking a link

document.querySelectorAll(".nav-links a").forEach((link) => {

    link.addEventListener("click", () => {
        navLinks.classList.remove("mobile-active");
    });

});


// ========================================
// NAVBAR SCROLL EFFECT
// ========================================

const navbar = document.querySelector(".navbar");

window.addEventListener("scroll", () => {

    if (window.scrollY > 30) {
        navbar.style.background = "rgba(7,7,7,.92)";
    } else {
        navbar.style.background = "rgba(7,7,7,.75)";
    }

});


// ========================================
// CURRENT YEAR
// ========================================

const year = new Date().getFullYear();

const copyright = document.querySelector(".copyright");

if (copyright) {
    copyright.textContent =
        `© ${year} Arif Shahariar Rafi`;
}
