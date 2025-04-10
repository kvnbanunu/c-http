document.addEventListener("DOMContentLoaded", () => {
    const keyE = document.getElementById("postkey");
    const valE = document.getElementById("postvalue");
    const postbutton = document.getElementById("postbutton");

    postbutton.onclick = async () => {
        const key = keyE.value;
        const val = valE.value;
        try {
            const response = await fetch("http://192.168.0.117:8080", {
                method: "POST",
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded",
                },
                body: new URLSearchParams({ key: val })
            });

            if (response.ok) {
                alert("Post success");
            }

        } catch (err) {
            console.error(err);
        }
    };
});
