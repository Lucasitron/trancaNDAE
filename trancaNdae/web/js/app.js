// Bootstrap: composição MVC (gerenciador de senhas).
import { SenhasView } from "./views/SenhasView.js";
import { SenhasController } from "./controllers/SenhasController.js";

new SenhasController(new SenhasView());
