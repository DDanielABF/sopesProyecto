import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import { RemoteComponent } from './remote.component';
import { MetricsComponent } from '../metrics/metrics.component';

export const routes: Routes = [
  {
    path: 'remote',
    component: RemoteComponent,
  },
  {
    path: 'metrics',
    component: MetricsComponent,
  },
];
