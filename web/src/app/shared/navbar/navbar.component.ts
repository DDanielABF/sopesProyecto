import { Component } from '@angular/core';
import { SharedModule } from '../shared.module';
import { MatToolbarModule } from '@angular/material/toolbar';
import { StorageService } from '../service/storage.service';
import { Router, RouterLink } from '@angular/router';

@Component({
  selector: 'app-navbar',
  imports: [SharedModule, MatToolbarModule, RouterLink],
  templateUrl: './navbar.component.html',
  styleUrl: './navbar.component.scss',
  standalone: true,
})
export class NavbarComponent {
  constructor(public storage: StorageService, private router: Router) {}

  logout() {
    this.storage.clearUser();
    this.router.navigate(['/login'], { replaceUrl: true });
  }
}
